/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "util/ledger_pb.h"

#include "cloud/cloud.pb.h"
#include "cloud/ledger.pb.h"

#include <pb_encode.h>
#include <pb_decode.h>

#include <stdexcept>
#include <cstring>

#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_LEDGER(_name) particle_cloud_ledger_##_name

namespace particle::test {

namespace {

template<typename T>
std::string encodeMessage(const pb_msgdesc_t* desc, const T& msg) {
    // Callbacks are invoked twice so they must not modify any state
    pb_ostream_t sizing = PB_OSTREAM_SIZING;
    if (!pb_encode(&sizing, desc, &msg)) {
        throw std::runtime_error("Failed to encode message");
    }
    std::string buf(sizing.bytes_written, '\0');
    auto s = pb_ostream_from_buffer((pb_byte_t*)buf.data(), buf.size());
    if (!pb_encode(&s, desc, &msg)) {
        throw std::runtime_error("Failed to encode message");
    }
    return buf;
}

bool readString(pb_istream_t* stream, std::string* str) {
    std::string s(stream->bytes_left, '\0');
    if (!pb_read(stream, (pb_byte_t*)s.data(), s.size())) {
        return false;
    }
    *str = std::move(s);
    return true;
}

template<size_t N>
void copyName(char (&dest)[N], const std::string& src) {
    if (src.size() >= N) {
        throw std::runtime_error("Ledger name is too long");
    }
    std::memcpy(dest, src.c_str(), src.size() + 1);
}

bool decodeGetInfoRequest(pb_istream_t* stream, DecodedRequest* req) {
    PB_LEDGER(GetInfoRequest) pbReq = {};
    pbReq.ledgers.arg = req;
    pbReq.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto req = (DecodedRequest*)*arg;
        std::string name;
        if (!readString(stream, &name)) {
            return false;
        }
        req->ledgerNames.push_back(std::move(name));
        return true;
    };
    return pb_decode(stream, &PB_LEDGER(GetInfoRequest_msg), &pbReq);
}

bool decodeSubscribeRequest(pb_istream_t* stream, DecodedRequest* req) {
    PB_LEDGER(SubscribeRequest) pbReq = {};
    pbReq.ledgers.arg = req;
    pbReq.ledgers.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        auto req = (DecodedRequest*)*arg;
        PB_LEDGER(SubscribeRequest_Ledger) pbLedger = {};
        if (!pb_decode(stream, &PB_LEDGER(SubscribeRequest_Ledger_msg), &pbLedger)) {
            return false;
        }
        req->ledgerNames.push_back(pbLedger.name);
        return true;
    };
    return pb_decode(stream, &PB_LEDGER(SubscribeRequest_msg), &pbReq);
}

bool decodeSetDataRequest(pb_istream_t* stream, DecodedRequest* req) {
    PB_LEDGER(SetDataRequest) pbReq = {};
    pbReq.data.arg = &req->data;
    pbReq.data.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        return readString(stream, (std::string*)*arg);
    };
    if (!pb_decode(stream, &PB_LEDGER(SetDataRequest_msg), &pbReq)) {
        return false;
    }
    req->ledgerNames.push_back(pbReq.name);
    return true;
}

bool decodeGetDataRequest(pb_istream_t* stream, DecodedRequest* req) {
    PB_LEDGER(GetDataRequest) pbReq = {};
    if (!pb_decode(stream, &PB_LEDGER(GetDataRequest_msg), &pbReq)) {
        return false;
    }
    req->ledgerNames.push_back(pbReq.name);
    return true;
}

} // namespace

DecodedRequest decodeRequest(const std::string& payload) {
    // The request data is a oneof of submessages with callback fields, which nanopb can't decode
    // in one go, so the fields of the outer message are iterated manually
    DecodedRequest req;
    auto s = pb_istream_from_buffer((const pb_byte_t*)payload.data(), payload.size());
    for (;;) {
        auto type = pb_wire_type_t();
        uint32_t tag = 0;
        bool eof = false;
        if (!pb_decode_tag(&s, &type, &tag, &eof)) {
            if (eof) {
                break;
            }
            throw std::runtime_error("Failed to decode request");
        }
        bool ok = true;
        if (tag == PB_CLOUD(Request_type_tag)) {
            uint64_t v = 0;
            ok = pb_decode_varint(&s, &v);
            req.type = v;
        } else if (type == PB_WT_STRING) {
            pb_istream_t sub = {};
            if (!pb_make_string_substream(&s, &sub)) {
                throw std::runtime_error("Failed to decode request");
            }
            switch (tag) {
            case PB_CLOUD(Request_ledger_get_info_tag):
                ok = decodeGetInfoRequest(&sub, &req);
                break;
            case PB_CLOUD(Request_ledger_subscribe_tag):
                ok = decodeSubscribeRequest(&sub, &req);
                break;
            case PB_CLOUD(Request_ledger_set_data_tag):
                ok = decodeSetDataRequest(&sub, &req);
                break;
            case PB_CLOUD(Request_ledger_get_data_tag):
                ok = decodeGetDataRequest(&sub, &req);
                break;
            default:
                break;
            }
            ok = pb_close_string_substream(&s, &sub) && ok;
        } else {
            ok = pb_skip_field(&s, type);
        }
        if (!ok) {
            throw std::runtime_error("Failed to decode request");
        }
    }
    return req;
}

DecodedResponse decodeResponse(const std::string& payload) {
    DecodedResponse resp;
    PB_CLOUD(Response) pbResp = {};
    pbResp.message.arg = &resp.message;
    pbResp.message.funcs.decode = [](pb_istream_t* stream, const pb_field_iter_t* /* field */, void** arg) {
        return readString(stream, (std::string*)*arg);
    };
    auto s = pb_istream_from_buffer((const pb_byte_t*)payload.data(), payload.size());
    if (!pb_decode(&s, &PB_CLOUD(Response_msg), &pbResp)) {
        throw std::runtime_error("Failed to decode response");
    }
    resp.result = pbResp.result;
    return resp;
}

std::string encodeResponse(int result) {
    PB_CLOUD(Response) pbResp = {};
    pbResp.result = result;
    return encodeMessage(&PB_CLOUD(Response_msg), pbResp);
}

std::string encodeGetInfoResponse(const std::vector<LedgerInfoEntry>& ledgers) {
    PB_CLOUD(Response) pbResp = {};
    pbResp.which_data = PB_CLOUD(Response_ledger_get_info_tag);
    pbResp.data.ledger_get_info.ledgers.arg = const_cast<std::vector<LedgerInfoEntry>*>(&ledgers);
    pbResp.data.ledger_get_info.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto ledgers = (const std::vector<LedgerInfoEntry>*)*arg;
        for (auto& l: *ledgers) {
            PB_LEDGER(GetInfoResponse_Ledger) pbLedger = {};
            copyName(pbLedger.name, l.name);
            if (l.scopeId.size() > sizeof(pbLedger.scope_id.bytes)) {
                return false;
            }
            std::memcpy(pbLedger.scope_id.bytes, l.scopeId.data(), l.scopeId.size());
            pbLedger.scope_id.size = l.scopeId.size();
            pbLedger.scope_type = (PB_LEDGER(ScopeType))l.scopeType;
            pbLedger.sync_direction = (PB_LEDGER(SyncDirection))l.syncDir;
            if (!pb_encode_tag_for_field(stream, field) ||
                    !pb_encode_submessage(stream, &PB_LEDGER(GetInfoResponse_Ledger_msg), &pbLedger)) {
                return false;
            }
        }
        return true;
    };
    return encodeMessage(&PB_CLOUD(Response_msg), pbResp);
}

std::string encodeSubscribeResponse(const std::vector<LedgerUpdateEntry>& ledgers) {
    PB_CLOUD(Response) pbResp = {};
    pbResp.which_data = PB_CLOUD(Response_ledger_subscribe_tag);
    pbResp.data.ledger_subscribe.ledgers.arg = const_cast<std::vector<LedgerUpdateEntry>*>(&ledgers);
    pbResp.data.ledger_subscribe.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto ledgers = (const std::vector<LedgerUpdateEntry>*)*arg;
        for (auto& l: *ledgers) {
            PB_LEDGER(SubscribeResponse_Ledger) pbLedger = {};
            copyName(pbLedger.name, l.name);
            if (l.lastUpdated) {
                pbLedger.has_last_updated = true;
                pbLedger.last_updated = l.lastUpdated;
            }
            if (!pb_encode_tag_for_field(stream, field) ||
                    !pb_encode_submessage(stream, &PB_LEDGER(SubscribeResponse_Ledger_msg), &pbLedger)) {
                return false;
            }
        }
        return true;
    };
    return encodeMessage(&PB_CLOUD(Response_msg), pbResp);
}

std::string encodeGetDataResponse(uint64_t lastUpdated, const std::string& data) {
    PB_CLOUD(Response) pbResp = {};
    pbResp.which_data = PB_CLOUD(Response_ledger_get_data_tag);
    auto& pbData = pbResp.data.ledger_get_data;
    pbData.has_last_updated = true;
    pbData.last_updated = lastUpdated;
    pbData.data.arg = const_cast<std::string*>(&data);
    pbData.data.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto data = (const std::string*)*arg;
        return pb_encode_tag_for_field(stream, field) &&
                pb_encode_string(stream, (const pb_byte_t*)data->data(), data->size());
    };
    return encodeMessage(&PB_CLOUD(Response_msg), pbResp);
}

std::string encodeRequest(int type) {
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = (PB_CLOUD(Request_Type))type;
    return encodeMessage(&PB_CLOUD(Request_msg), pbReq);
}

std::string encodeNotifyUpdateRequest(const std::vector<LedgerUpdateEntry>& ledgers) {
    PB_CLOUD(Request) pbReq = {};
    pbReq.type = PB_CLOUD(Request_Type_LEDGER_NOTIFY_UPDATE);
    pbReq.which_data = PB_CLOUD(Request_ledger_notify_update_tag);
    pbReq.data.ledger_notify_update.ledgers.arg = const_cast<std::vector<LedgerUpdateEntry>*>(&ledgers);
    pbReq.data.ledger_notify_update.ledgers.funcs.encode = [](pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg) {
        auto ledgers = (const std::vector<LedgerUpdateEntry>*)*arg;
        for (auto& l: *ledgers) {
            PB_LEDGER(NotifyUpdateRequest_Ledger) pbLedger = {};
            copyName(pbLedger.name, l.name);
            pbLedger.last_updated = l.lastUpdated;
            if (!pb_encode_tag_for_field(stream, field) ||
                    !pb_encode_submessage(stream, &PB_LEDGER(NotifyUpdateRequest_Ledger_msg), &pbLedger)) {
                return false;
            }
        }
        return true;
    };
    return encodeMessage(&PB_CLOUD(Request_msg), pbReq);
}

} // namespace particle::test
