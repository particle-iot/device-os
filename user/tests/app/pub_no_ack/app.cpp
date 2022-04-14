#include "application.h"

struct DataUsage : public Printable {
    int cid = 0;
    int tx_sess_bytes = 0;
    int rx_sess_bytes = 0;
    int tx_total_bytes = 0;
    int rx_total_bytes = 0;


    static inline int _cbUGCNTRD(int type, const char* buf, int len, DataUsage* data)
    {
        if ((type == TYPE_PLUS) && data) {
            int a,b,c,d,e;
            // +UGCNTRD: 31,2704,1819,2724,1839\r\n
            // +UGCNTRD: <cid>,<tx_sess_bytes>,<rx_sess_bytes>,<tx_total_bytes>,<rx_total_bytes>
            if (sscanf(buf, "\r\n+UGCNTRD: %d,%d,%d,%d,%d\r\n", &a,&b,&c,&d,&e) == 5) {
                data->cid = a;
                data->tx_sess_bytes = b;
                data->rx_sess_bytes = c;
                data->tx_total_bytes = d;
                data->rx_total_bytes = e;
            }
        }
        return WAIT;
    }

    static bool sample_usage(DataUsage& _data_usage)
    {
    		return (RESP_OK == Cellular.command(_cbUGCNTRD, &_data_usage, 10000, "AT+UGCNTRD\r\n"));
    }

    DataUsage(bool sample=true)
    {
    		if (sample)
    			sample_usage(*this);
    }

    size_t printTo(Print& p) const override
    {
        return p.printlnf("CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d\r\n",
            cid,
            tx_sess_bytes, rx_sess_bytes,
            tx_total_bytes, rx_total_bytes);
    }

    void subtract(DataUsage& usage)
    {
    	    tx_total_bytes -= usage.tx_total_bytes;
        rx_total_bytes -= usage.rx_total_bytes;
    }

    int total() { return rx_total_bytes + tx_total_bytes; }
};



template <typename T> int measure(T t, int timeout, Print& stream)
{
	DataUsage before;
	t();
	delay(timeout);
	DataUsage after;
	after.subtract(before);
	return after.total();
}


void publish_ack()
{
	Particle.publish("a","b");
}

void publish_nack()
{
	Particle.publish("a","b", NO_ACK);
}

void setup()
{
	Serial.begin(9600);
	Serial.println("Publish data use tester.");
	Serial.println("Press 'p' to publish with ACK");
	Serial.println("Press 'P' to publish without ACK");
	Serial.println("Press 'D' to print data usage counters.");
	Serial.println("Each publish waits 30s before collecting the data count.");
}

void loop()
{
	const int timeout = 30*1000;
	char c = Serial.read();
	switch (c)
	{
	case 'p':
		Serial.print("Publishing with ACK...");
		Serial.printlnf("%d bytes", measure(publish_ack, timeout, Serial));
		break;
	case 'P':
		Serial.print("Publishing with NACK...");
		Serial.printlnf("%d bytes", measure(publish_nack, timeout, Serial));
		break;
	case 'D':
		Serial.println(DataUsage());
		break;
	}
}

