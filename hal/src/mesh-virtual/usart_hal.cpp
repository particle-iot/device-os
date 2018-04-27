/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* NOTE: order is important */
#include <boost/system/system_error.hpp>
#include "boost_asio_wrap.h"
#include "usart_hal.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include "service_debug.h"
#include "ringbuf_helper.h"
#include "device_config.h"

namespace asio = boost::asio;

namespace {

uint8_t HAL_USART_Calculate_Word_Length(uint32_t config, uint8_t noparity)
{
  uint8_t wlen = 0;
  switch (config & SERIAL_DATA_BITS) {
    case SERIAL_DATA_BITS_7:
      wlen += 7;
      break;
    case SERIAL_DATA_BITS_8:
      wlen += 8;
      break;
    case SERIAL_DATA_BITS_9:
      wlen += 9;
      break;
  }

  if ((config & SERIAL_PARITY) && !noparity)
    wlen++;

  if (wlen > 9 || wlen < (noparity ? 7 : 8))
    wlen = 0;

  return wlen;
}

uint32_t HAL_USART_Calculate_Data_Bits_Mask(uint32_t config)
{
  return (1 << HAL_USART_Calculate_Word_Length(config, 1)) - 1;
}


class UsartRunner {
public:
  UsartRunner() {
  }

  ~UsartRunner() {
    io_.stop();
    if (thread_) {
      thread_->join();
    }
  }

  void run() {
    thread_.reset(new std::thread([&]{ asio::io_service::work wrk(io_); io_.run(); }));
  }

  asio::io_service& getIoService() {
    return io_;
  }

private:
  asio::io_service io_;
  std::unique_ptr<std::thread> thread_;
};

UsartRunner usartRunner;

void compact_buffer(uint16_t* ptr, size_t len) {
  uint8_t* u8buf = (uint8_t*)ptr;
  for (size_t i = 0; i < len; i++) {
    const uint8_t v = (const uint8_t)ptr[i];
    u8buf[i] = v;
  }
}

void uncompact_buffer(uint16_t* ptr, size_t len) {
  uint8_t* u8buf = (uint8_t*)ptr;
  std::vector<uint8_t> tmp;
  tmp.resize(len);
  memcpy(tmp.data(), u8buf, len);
  for (size_t i = 0; i < len; i++) {
    ptr[i] = tmp[i];
  }
}

class Usart {
public:
  Usart(const std::string& name) :
      name_(name),
      serial_(usartRunner.getIoService()) {
    tmpBuf_.resize(SERIAL_BUFFER_SIZE);
  }
  ~Usart() {
  }

  void init(Ring_Buffer* rx, Ring_Buffer* tx);
  void begin(uint32_t baud);
  void end();
  uint32_t writeData(uint8_t data);
  int32_t availableDataForWrite();
  int32_t availableData();
  int32_t readData();
  int32_t peekData();
  void flushData();
  bool isEnabled();
  void halfDuplex(bool enable);
  void beginConfig(uint32_t baud, uint32_t config);
  uint32_t writeNineBitData(uint16_t data);
  void sendBreak();
  uint8_t breakDetected();

private:
  int32_t availableForWrite();
  bool validateConfig(uint32_t config) const;
  void doWrite();
  void onWriteComplete(const boost::system::error_code& ec, std::size_t bytes);
  void doRead();
  void onReadComplete(const boost::system::error_code& ec, std::size_t bytes);
  void onOverflowReadComplete(const boost::system::error_code& ec, std::size_t bytes);

private:
  std::string name_;
  asio::serial_port serial_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool inWrite_ = false;
  bool inRead_ = false;

  Ring_Buffer* rxBuf_ = nullptr;
  Ring_Buffer* txBuf_ = nullptr;
  bool enabled_ = false;
  uint32_t config_ = 0;

  std::vector<uint8_t> tmpBuf_;
};

std::vector<std::unique_ptr<Usart> > usartMap;

void Usart::init(Ring_Buffer* rx, Ring_Buffer* tx) {
  std::lock_guard<std::mutex> lk(mutex_);
  rxBuf_ = rx;
  txBuf_ = tx;

  if (rx) {
    memset(rx, 0, sizeof(Ring_Buffer));
  }

  if (tx) {
    memset(tx, 0, sizeof(Ring_Buffer));
  }
}

void Usart::begin(uint32_t baud) {
  beginConfig(baud, 0);
}

void Usart::end() {
  flushData();
  {
    std::lock_guard<std::mutex> lk(mutex_);
    boost::system::error_code ec;
    serial_.cancel(ec);
    serial_.close(ec);

    if (rxBuf_) {
      memset(rxBuf_, 0, sizeof(Ring_Buffer));
    }

    if (txBuf_) {
      memset(txBuf_, 0, sizeof(Ring_Buffer));
    }

    enabled_ = false;
    inWrite_ = false;
    inRead_ = false;

    cv_.notify_all();
  }
}

uint32_t Usart::writeData(uint8_t data) {
  return writeNineBitData(data);
}

int32_t Usart::availableDataForWrite() {
  std::lock_guard<std::mutex> lk(mutex_);
  return availableForWrite();
}

int32_t Usart::availableForWrite() {
  int32_t tail = txBuf_->tail;
  int32_t available = SERIAL_BUFFER_SIZE - (txBuf_->head >= tail ?
    txBuf_->head - tail :
    (SERIAL_BUFFER_SIZE + txBuf_->head - tail) - 1);

  return available;
}

int32_t Usart::availableData() {
  std::lock_guard<std::mutex> lk(mutex_);
  return (unsigned int)(SERIAL_BUFFER_SIZE + rxBuf_->head - rxBuf_->tail) % SERIAL_BUFFER_SIZE;
}

int32_t Usart::readData() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (rxBuf_->head != rxBuf_->tail) {
    uint16_t c = rxBuf_->buffer[rxBuf_->tail];
    rxBuf_->tail = (unsigned int)(rxBuf_->tail + 1) % SERIAL_BUFFER_SIZE;
    return c;
  }

  return -1;
}

int32_t Usart::peekData() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (rxBuf_->head != rxBuf_->tail) {
    uint16_t c = rxBuf_->buffer[rxBuf_->tail];
    return c;
  }

  return -1;
}

void Usart::flushData() {
  /* FIXME: Not implemented */
}

bool Usart::isEnabled() {
  std::lock_guard<std::mutex> lk(mutex_);
  return enabled_;
}

void Usart::halfDuplex(bool enable) {
  /* NOT IMPLEMENTED */
}

bool Usart::validateConfig(uint32_t config) const {
  if (name_.empty()) {
    /* No mapping was specified */
    return false;
  }

  // Total word length should be either 7 or 8 bytes
  const auto wl = HAL_USART_Calculate_Word_Length(config, 0);
  if (wl < 7 || wl > 8) {
    return false;
  }

  // Either No, Even or Odd parity
  if ((config & SERIAL_PARITY) == (SERIAL_PARITY_EVEN | SERIAL_PARITY_ODD)) {
    return false;
  }

  if ((config & SERIAL_HALF_DUPLEX) || (config & LIN_MODE)) {
    /* Half-duplex is not supported */
    /* LIN-mode is not supported */
    return false;
  }

  if ((config & SERIAL_STOP_BITS) == SERIAL_STOP_BITS_0_5) {
    return false;
  }

  if ((config & SERIAL_FLOW_CONTROL) && (config & SERIAL_FLOW_CONTROL) != SERIAL_FLOW_CONTROL_RTS_CTS) {
    return false;
  }

  return true;
}

void Usart::beginConfig(uint32_t baud, uint32_t config) {
  if (!validateConfig(config)) {
    LOG(ERROR, "Failed to initialize UART: invalid config supplied");
    end();
    return;
  }

  using spb = asio::serial_port_base;

  std::lock_guard<std::mutex> lk(mutex_);

  boost::system::error_code ec;
  serial_.open(name_, ec);
  if (ec) {
    LOG(ERROR, "Failed to open UART: %s", ec.message().c_str());
    end();
    return;
  }

  serial_.set_option(asio::serial_port_base::baud_rate(baud));
  serial_.set_option(asio::serial_port_base::character_size(HAL_USART_Calculate_Word_Length(config, 0)));
  switch (config & SERIAL_PARITY) {
    case SERIAL_PARITY_NO: {
      serial_.set_option(spb::parity(spb::parity::none));
      break;
    }
    case SERIAL_PARITY_EVEN: {
      serial_.set_option(spb::parity(spb::parity::odd));
      break;
    }
    case SERIAL_PARITY_ODD: {
      serial_.set_option(spb::parity(spb::parity::even));
      break;
    }
  }

  switch (config & SERIAL_STOP_BITS) {
    case SERIAL_STOP_BITS_1: {
      serial_.set_option(spb::stop_bits(spb::stop_bits::one));
      break;
    }
    case SERIAL_STOP_BITS_1_5: {
      serial_.set_option(spb::stop_bits(spb::stop_bits::onepointfive));
      break;
    }
    case SERIAL_STOP_BITS_2: {
      serial_.set_option(spb::stop_bits(spb::stop_bits::two));
      break;
    }
  }

  if ((config & SERIAL_FLOW_CONTROL)) {
    serial_.set_option(spb::flow_control(spb::flow_control::hardware));
  } else {
    serial_.set_option(spb::flow_control(spb::flow_control::none));
  }

  enabled_ = true;
  LOG(INFO, "UART started");
  doRead();
}

void Usart::doRead() {
  if (!inRead_) {
    inRead_ = true;
    /* TODO: flow control */
    size_t len = ring_space_contig(SERIAL_BUFFER_SIZE, rxBuf_->head, rxBuf_->tail);
    uint16_t* data = rxBuf_->buffer + rxBuf_->head;
    uint8_t* u8data = (uint8_t*)data;
    if (len > 0) {
      serial_.async_read_some(asio::buffer(u8data, len),
                              std::bind(&Usart::onReadComplete, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    } else {
      serial_.async_read_some(asio::buffer(tmpBuf_.data(), 1),
                              std::bind(&Usart::onOverflowReadComplete, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    }
  }
}

void Usart::onReadComplete(const boost::system::error_code& ec, std::size_t bytes) {
  if (!ec) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (bytes > 0) {
      uncompact_buffer(rxBuf_->buffer + rxBuf_->head, bytes);
      rxBuf_->head += bytes;
      rxBuf_->head %= SERIAL_BUFFER_SIZE;
    }
    inRead_ = false;
    doRead();
  } else {
    end();
  }
}

void Usart::onOverflowReadComplete(const boost::system::error_code& ec, std::size_t bytes) {
  if (!ec) {
    std::lock_guard<std::mutex> lk(mutex_);
    LOG(WARN, "UART RX buffer overflow");
    inRead_ = false;
    doRead();
  } else {
    end();
  }
}

uint32_t Usart::writeNineBitData(uint16_t data) {
  /* Remove any bits exceeding data bits configured */
  data &= HAL_USART_Calculate_Data_Bits_Mask(config_);

  {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [&]{ return availableForWrite() > 0 || !enabled_; });

    if (availableForWrite() > 0) {
      unsigned i = (txBuf_->head + 1) % SERIAL_BUFFER_SIZE;
      txBuf_->buffer[txBuf_->head] = data;
      txBuf_->head = i;
      doWrite();
    } else {
      return 0;
    }
  }

  return 1;
}

void Usart::sendBreak() {
  /* NOT IMPLEMENTED */
}

uint8_t Usart::breakDetected() {
  /* NOT IMPLEMENTED */
  return 0;
}

void Usart::doWrite() {
  if (!inWrite_) {
    if (txBuf_->head != txBuf_->tail) {
      inWrite_ = true;
      size_t len = ring_data_contig(SERIAL_BUFFER_SIZE, txBuf_->head, txBuf_->tail);
      uint16_t* data = txBuf_->buffer + txBuf_->tail;
      compact_buffer(data, len);
      const uint8_t* u8data = (const uint8_t*)data;
      asio::async_write(serial_, asio::buffer(u8data, len),
                        std::bind(&Usart::onWriteComplete, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
    }
  }
}

void Usart::onWriteComplete(const boost::system::error_code& ec, std::size_t bytes) {
  if (!ec) {
    std::lock_guard<std::mutex> lk(mutex_);
    txBuf_->tail += bytes;
    txBuf_->tail %= SERIAL_BUFFER_SIZE;

    inWrite_ = false;
    cv_.notify_all();
    doWrite();
  } else {
    end();
  }
}

} /* anonymous */

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer* rx_buffer, Ring_Buffer* tx_buffer) {
  static std::once_flag f;
  std::call_once(f, []() {
    for(int i = 0; i < TOTAL_USARTS; i++) {
      usartMap.push_back(std::unique_ptr<Usart>(new Usart(deviceConfig.usart_map[i])));
    }
    usartRunner.run();
  });

  usartMap[serial]->init(rx_buffer, tx_buffer);
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud) {
  usartMap[serial]->begin(baud);
}

void HAL_USART_End(HAL_USART_Serial serial) {
  usartMap[serial]->end();
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data) {
  return usartMap[serial]->writeData(data);
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial) {
  return usartMap[serial]->availableDataForWrite();
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial) {
  return usartMap[serial]->availableData();
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial) {
  return usartMap[serial]->readData();
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial) {
  return usartMap[serial]->peekData();
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial) {
  return usartMap[serial]->flushData();
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial) {
  return usartMap[serial]->isEnabled();
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool enable) {
  usartMap[serial]->halfDuplex(enable);
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void*) {
  usartMap[serial]->beginConfig(baud, config);
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data) {
  return usartMap[serial]->writeNineBitData(data);
}

void HAL_USART_Send_Break(HAL_USART_Serial serial, void* reserved) {
  return usartMap[serial]->sendBreak();
}

uint8_t HAL_USART_Break_Detected(HAL_USART_Serial serial) {
  return usartMap[serial]->breakDetected();
}
