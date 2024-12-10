#include <algorithm>
#include <cstring>

#include "application.h"
#include "test.h"

#include "random.h"

namespace {

bool checkStatus(const CloudEvent& ev, CloudEvent::Status status) {
    if (ev.status() != status) {
        return false;
    }
    if (ev.isNew() != (status == CloudEvent::NEW)) {
        return false;
    }
    if (ev.isSending() != (status == CloudEvent::SENDING)) {
        return false;
    }
    if (ev.isSent() != (status == CloudEvent::SENT)) {
        return false;
    }
    if (ev.isValid() == (status == CloudEvent::INVALID)) {
        return false;
    }
    if (ev.isOk() == (status == CloudEvent::FAILED || status == CloudEvent::INVALID)) {
        return false;
    }
    return true;
}

bool checkFailed(const CloudEvent& ev, int error) {
    if (!checkStatus(ev, CloudEvent::FAILED)) {
        return false;
    }
    if (ev.error() != error) {
        return false;
    }
    return true;
}

bool waitStatus(const CloudEvent& ev, CloudEvent::Status status, unsigned timeout) {
    auto t1 = millis();
    for (;;) {
        if (ev.status() == status) {
            if (!checkStatus(ev, status)) { // Check the shorthand methods too
                return false;
            }
            return true;
        }
        if (!ev.isOk() || millis() - t1 >= timeout) {
            return false;
        }
        delay(100);
    }
}

bool checkData(CloudEvent& ev, const Buffer& data) {
    size_t size = ev.size();
    if (size != data.size()) {
        return false;
    }
    size_t oldPos = ev.pos();
    if (ev.seek(0) != 0) {
        return false;
    }
    char buf[128];
    size_t offs = 0;
    while (offs < size) {
        size_t n = std::min(size - offs, sizeof(buf));
        if (ev.read(buf, n) != (int)n) {
            return false;
        }
        if (std::memcmp(buf, data.data() + offs, n) != 0) {
            return false;
        }
        offs += n;
    }
    if (ev.seek(oldPos) != (int)oldPos) {
        return false;
    }
    return true;
}

bool genRandom(Buffer& buf, size_t size) {
    if (!buf.resize(size)) {
        return false;
    }
    Random::genSecure(buf.data(), size);
    return true;
}

} // namespace

// TODO: Many of these tests should really be unit tests

test(01_initial_event_state) {
    CloudEvent ev;
    assertEqual(std::strcmp(ev.name(), ""), 0);
    assertTrue(ev.contentType() == ContentType::TEXT);
    assertTrue(ev.data() == Buffer());
    assertTrue(ev.dataString() == String());
    assertTrue(ev.dataStructured() == EventData());
    assertEqual(ev.size(), 0);
    assertTrue(ev.isEmpty());
    assertEqual(ev.pos(), 0);
    assertTrue(checkStatus(ev, CloudEvent::NEW));
    assertEqual(ev.error(), 0);
}

test(02_set_get_event_properties) {
    // Name
    {
        CloudEvent ev;
        ev.name("valid_name");
        assertEqual(std::strcmp(ev.name(), "valid_name"), 0);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    {
        CloudEvent ev;
        ev.name("very_looooooooooooooooooooooooooooooooooooooooooooooooooong_name"); // 64 characters
        assertEqual(std::strcmp(ev.name(), "very_looooooooooooooooooooooooooooooooooooooooooooooooooong_name"), 0);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    {
        CloudEvent ev;
        ev.name("");
        assertEqual(std::strcmp(ev.name(), ""), 0);
        assertTrue(checkFailed(ev, Error::INVALID_ARGUMENT));
    }
    {
        CloudEvent ev;
        ev.name("too_looooooooooooooooooooooooooooooooooooooooooooooooooooong_name"); // 65 characters
        assertEqual(std::strcmp(ev.name(), ""), 0);
        assertTrue(checkFailed(ev, Error::INVALID_ARGUMENT));
    }
    // Content type
    {
        CloudEvent ev;
        ev.contentType(ContentType::BINARY);
        assertTrue(ev.contentType() == ContentType::BINARY);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    // Data
    {
        // Buffer
        CloudEvent ev;
        ev.data(Buffer::fromHex("0123456789abcdef"));
        assertTrue(ev.data() == Buffer::fromHex("0123456789abcdef"));
        assertEqual(ev.size(), 8);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    {
        // String
        CloudEvent ev;
        ev.data(String("abc"));
        assertTrue(ev.dataString() == String("abc"));
        assertTrue(ev.data() == Buffer("abc", 3));
        assertEqual(ev.size(), 3);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    {
        // EventData (Variant)
        CloudEvent ev;
        ev.data(VariantMap{ { "a", 1 }, { "b", 2 } });
        assertTrue((ev.dataStructured() == VariantMap{ { "a", 1 }, { "b", 2 } }));
        assertTrue(ev.data() == Buffer::fromHex("a2616101616202"));
        assertEqual(ev.size(), 7);
        assertTrue(ev.contentType() == ContentType::STRUCTURED);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
}

test(03_read_write_event_data_stream) {
    // Multiple writes
    {
        CloudEvent ev;
        assertTrue(ev.data() == Buffer());
        assertEqual(ev.write((const char*)nullptr, 0), 0);
        assertTrue(ev.data() == Buffer());
        assertEqual(ev.write("\x01", 1), 1);
        assertTrue(ev.data() == Buffer::fromHex("01"));
        assertEqual(ev.write("\x23\x45\x67", 3), 3);
        assertTrue(ev.data() == Buffer::fromHex("01234567"));
        assertEqual(ev.write("\x89\xab\xcd\xef", 4), 4);
        assertTrue(ev.data() == Buffer::fromHex("0123456789abcdef"));
        assertEqual(ev.pos(), 8);
        assertEqual(ev.size(), 8);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    // Maximum data size in one go
    {
        CloudEvent ev;
        size_t size = CloudEvent::MAX_SIZE;
        assertEqual(size, 16384);
        Buffer buf;
        assertTrue(genRandom(buf, size));
        assertEqual(ev.write(buf.data(), buf.size()), size);
        assertEqual(ev.size(), size);
        assertEqual(ev.pos(), size);
        assertTrue(checkData(ev, buf));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
        // Try writing one more byte
        assertEqual(ev.write(buf.data(), 1), (int)Error::COAP_TOO_LARGE_PAYLOAD);
        assertTrue(checkFailed(ev, Error::COAP_TOO_LARGE_PAYLOAD));
    }
    // Maximum data size in multiple writes
    {
        CloudEvent ev;
        size_t size = CloudEvent::MAX_SIZE;
        assertEqual(size, 16384);
        Buffer buf;
        assertTrue(genRandom(buf, size));
        size_t chunkSize = 100;
        size_t offs = 0;
        while (offs < size) {
            size_t n = std::min(size - offs, chunkSize);
            assertEqual(ev.write(buf.data() + offs, n), n);
            offs += n;
        }
        assertEqual(ev.size(), size);
        assertEqual(ev.pos(), size);
        assertTrue(checkData(ev, buf));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
        // Try writing one more byte
        assertEqual(ev.write(buf.data(), 1), (int)Error::COAP_TOO_LARGE_PAYLOAD);
        assertTrue(checkFailed(ev, Error::COAP_TOO_LARGE_PAYLOAD));
    }
    // Random access to event data
    {
        CloudEvent ev;
        assertEqual(ev.write("\x11\x22\x33\x44\x55\x66\x77\x88", 8), 8);
        assertEqual(ev.size(), 8);
        assertEqual(ev.pos(), 8);
        assertTrue(ev.data() == Buffer::fromHex("1122334455667788"));
        // Reading from the middle of the data
        assertEqual(ev.seek(1), 1);
        assertEqual(ev.pos(), 1);
        Buffer buf;
        assertTrue(buf.resize(3));
        assertEqual(ev.read(buf.data(), 3), 3);
        assertEqual(ev.pos(), 4);
        assertTrue(buf == Buffer::fromHex("223344"));
        // Writing to the middle of the data
        assertEqual(ev.write("\xaa\xbb\xcc", 3), 3);
        assertEqual(ev.pos(), 7);
        assertTrue(ev.data() == Buffer::fromHex("11223344aabbcc88"));
        assertEqual(ev.pos(), 7); // data() doesn't modify the current position
        // Reading from the beginning of the data
        assertEqual(ev.seek(0), 0);
        assertEqual(ev.pos(), 0);
        assertTrue(buf.resize(1));
        assertEqual(ev.read(buf.data(), 1), 1);
        assertEqual(ev.pos(), 1);
        assertTrue(buf == Buffer::fromHex("11"));
        // Writing to the beginning of the data
        assertEqual(ev.seek(0), 0);
        assertEqual(ev.pos(), 0);
        assertEqual(ev.write("\xdd", 1), 1);
        assertEqual(ev.pos(), 1);
        assertTrue(ev.data() == Buffer::fromHex("dd223344aabbcc88"));
        // Reading from the end of the data
        assertEqual(ev.seek(7), 7);
        assertEqual(ev.pos(), 7);
        assertTrue(buf.resize(1));
        assertEqual(ev.read(buf.data(), 1), 1);
        assertEqual(ev.pos(), 8);
        assertTrue(buf == Buffer::fromHex("88"));
        // Writing to the end of the data
        assertEqual(ev.seek(7), 7);
        assertEqual(ev.pos(), 7);
        assertEqual(ev.write("\xee", 1), 1);
        assertEqual(ev.pos(), 8);
        assertTrue(ev.data() == Buffer::fromHex("dd223344aabbccee"));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    // Reading and writing past the end of the data
    {
        CloudEvent ev;
        assertEqual(ev.write("\x11\x22\x33"), 3);
        assertTrue(ev.data() == Buffer::fromHex("112233"));
        // Reading past the end of the data
        Buffer buf;
        assertTrue(buf.resize(3));
        assertEqual(ev.seek(1), 1);
        assertEqual(ev.read(buf.data(), 3), 2);
        assertEqual(ev.pos(), 3);
        assertTrue(buf == Buffer::fromHex("2233"));
        assertEqual(ev.read(buf.data(), 1), (int)Error::END_OF_STREAM);
        assertEqual(ev.pos(), 3);
        assertTrue(checkStatus(ev, CloudEvent::NEW));
        // Writing past the end of the data
        assertEqual(ev.seek(1), 1);
        assertEqual(ev.write("\xaa\xbb\xcc", 3), 3);
        assertEqual(ev.size(), 4);
        assertEqual(ev.pos(), 4);
        assertTrue(ev.data() == Buffer::fromHex("11aabbcc"));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
}

test(04_resize_event_data) {
    // Truncating the data
    {
        // RAM only
        CloudEvent ev;
        Buffer buf;
        assertTrue(genRandom(buf, 100));
        assertEqual(ev.write(buf.data(), 100), 100);
        assertEqual(ev.size(), 100);
        assertEqual(ev.pos(), 100);
        assertEqual(ev.setSize(50), 0);
        assertEqual(ev.size(), 50);
        assertEqual(ev.pos(), 50);
        assertTrue(checkData(ev, buf.slice(0, 50)));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
    {
        // RAM + file
        CloudEvent ev;
        size_t ramSize = ev.maxDataInRam();
        assertEqual(ramSize, 1024);
        size_t fullSize = ramSize * 3;
        Buffer buf;
        assertTrue(genRandom(buf, fullSize));
        assertEqual(ev.write(buf.data(), fullSize), fullSize);
        assertEqual(ev.size(), fullSize);
        assertEqual(ev.pos(), fullSize);
        size_t newSize = fullSize / 2;
        assertMore(newSize, ramSize);
        assertEqual(ev.setSize(newSize), 0);
        assertEqual(ev.size(), newSize);
        assertEqual(ev.pos(), newSize);
        assertTrue(checkData(ev, buf.slice(0, newSize)));
        assertTrue(checkStatus(ev, CloudEvent::NEW));
    }
}

test(05_connect_to_cloud) {
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 60000));
}

test(06_publish_text_event_with_polling_and_callback) {
    CloudEvent ev;
    ev.name("abc");
    ev.data("n7nuWm8oPlTegXFANsoCdnrI1Jz4oGQ3Vs3MuFG6BIvl3TXVl3PFst8BeRkZPfam4kB9RblSFH7Rwx6C1DmsK1Ctnp1DovdzkvSR"); // 100 characters
    Vector<CloudEvent::Status> callbackStatuses;
    callbackStatuses.append(ev.status());
    ev.onStatusChange([&](auto ev) {
        callbackStatuses.append(ev.status());
    });
    Particle.publish(ev);
    assertTrue(checkStatus(ev, CloudEvent::SENDING));
    assertTrue(waitStatus(ev, CloudEvent::SENT, 15000));
    assertTrue((callbackStatuses == Vector{ CloudEvent::NEW, CloudEvent::SENDING, CloudEvent::SENT }));
}

test(07_publish_binary_event_of_maximum_size) {
    CloudEvent ev;
    ev.name("abc");
    ev.data(Buffer::fromHex("ff8482032e330a53601242d279ee0eda3b0e9bee571e05dee7580d0962c9fda9c8a0e27f3e15de85fb63d8c2824b891760c7728e68fbcc4331ee3cd9b8da56d6a3ea80a22e78032a20b9bcd6b844281f4d0ab784fde4c9f15d92dc747692a3940238d3303dbc56e413afadc8add8383777c7c0abc6e57e3ab58a77c7f8fef5630243236fef061030e8ac82399cea185dba9f674ade367a0ba06b3257699866145f1a4662caefcc340a348ce68d06991d67cb1af93ded3cae622ea15f047d35f85f2ad42f22c706b002d69d1b033803ba7f4d46be06eaf64671f10c9c6f6372dfa84492381ce40ef65ee69d2a0ab3e33f178bea883fb8b8c3e7ee4e02feceeb1a99de001152a49d38a01b26b06c2d027d8361b6d1e0dd00867ee801873b0fc061c13384701f78124a02825c6fe2dfcd2f76305726d408e1765fb0299fc00255b6e2d28000d81105db912894eea5d3c73b71129938721e53002633006047d7fa8e3ea4a1ca7773a597e60efd265203e39411f523d2716e5ff02b32e90e5aae7194f8c5c5e62d5a08c416a0ca18c95588b4d431ef5309260e613bae3d32732a432f0035bdde3c8510236c62cd968efd656d3b5e59efb47ba946d8ac51ff81c5908953088052308296db0bce61d66a87dfa6541a13382fa2b55ba666df845d4458bf9b9a8abddc35263a2b11c55b6e634972acd99c38a5885a80230b2769fe3c3aa2f41845e71b95ff76f002e99490fe574f6b0a1aa322c7083651dcedf9168d272ad6e49d24eed7c2171918e381da1dde565673d188bde2cb07f1c96955c82f0f3ff1d020cdcd7599b98b7eadc76c551768b3591af5155d564e95afde4043461ab6a645b19890c12b562e05a1fdef95683f272f861bdef68a2a4ac27e2eb76556ef1662bd97b478e396e0c7dd4ca4cc53cf376b56151e12a97a5a2cb7401ada20eeb3a364fc013c78c7dd08dbfbf8573c25f6b4f55de4baef9b07e8a0625ebd5aa2e0ebf64522a16e56ea80fe6be686c27ac9ede4deea8d8cf33d9627bcfdec2754833ded30e6c2fbc6ac0240bb81e406928bd20e977aff7a2be4cd25ba6aba2fdc729e0d9031da6a26fe7f7c3f29246a9618c715dda5067b5b9141f0d5d3af3602dfc29231db23a108257e59daed5e4f3c07cae19caef9da5c33cf45c8a22656eafab6fa65e9446cb543ab92a6b2eca81c585757f2b8a2f10afd70ea000ddb0f73c3327fd4b77d8ddb887d729c1b208aa923e90e5127e1ab97ea04e0885d58c1d0964b72fe1ca705bd86f333efd771ae0db2a6736d3d0d3d38ea3cbdbe1f84dda39f545ab4894f109fc3208dccf2c52711a469d4c9bf32aa239e86a84eaa855e22fbca957b2810c4cfa2aba8f773ebf90d0741d59bcef2489a379f7e731931e84d5f426d4f80abac16c02537762a3a0b39958f8bc9652ceb10c5698f00c95f374665e6a6de7ae7c8c039543d5e728ffe8089466ad36224382f9991230d3af3cacbfd100dac48076218cec35ace951c4bf919eed2b5c0609b120e6e86c5b836bc12335cb404759c77091935f62ae1ae57806d0024ca0f9c75dd68778ff2a4e4084fcb668f17f444b91cc81859bb69467e0a4c2084a49b7908350a387ebc9f388f541b659b1e0f10aa326bfac7ed660706a8fb9e34c1c4ba4cf1105e824c88b3faa7faedd5488651d1cacd9b5147076195bf9711f34175bc4db084bc7a09990ce971005506c6f283b54f145254dea94bad2e383a33aa7de2d7d19ac5dd90b97761ccf3c3de6480c442202effa410a517dc4696365d0d16fc5a8f1733c5afaa9054b795b810b290858998192242023fc432ab9eec82ccf1a1b924b59726236fd3af2fa2a2e09b60e65926e87f5115754384fc7763f6626bf3673882cfc7da1769bb98fb353bdaef2782db1a355281d57fc3ac5f5c99427e46f1f2cee35ee5a03fe7d32a031a2a911fc500e51c71d471f7be9b6dee82c04fbe0273bc1ee4922e66e74d74e18197fb88a297a7604d066d67bf7f518e05d0cff607ec6f3d79fbeec7cba9e978974db0c5c9818f4cc0e661c815192f5e3088d7d80f7aa8ca8a01116deac89ecce17b2c346507190faeebe81965a1ee92d0c1f530c0f3fa5e9bea69c974927d3eb280435b2e5c2d580020c055dac01f13b46659142c44e3497f540061a4c3059e4cd6dca93cc248cff0c79d7759eff42000f591d46ea6ef4dc735a059746e22ab255848fd63717268642e97b64d322fb56caf728ecbe2c868c48fe2ca04156109b2813000602885c38b1ce10b3bb762c5029460522ccd023746a73f70730c707d7135138b46dc1e271730ab445e369cf72f1313d59a1707476e781f0dce766c61e6932544747af64722133d0a51233ddf2f2c84e3bedea5d7b08e6f9c4cbe16fafec35abd887dc9abefa83b4326a6d5c1c48fdbafd48547b8d6c51341d7cbdab7542c30d9ef98bb4d88ccdb34a2d8c547420a5f836e06a61f1b3e9a584e7c5d79670273ccb0a0d3f551e63249b1b9830997e873f984d5205e8b6cc1c3173cdd1f84c00e34e085f4b3674f55cbddcd5c9f4764311149ac6af7b3d4ceda118d4a25d0a1b05bfe31d4eae617efd88d6a57678bf3b71717f24dc1e11a70d88483fd20e0dec602be89f28e961e5a5dc3102d5e5598a21ff3a65207ba7c0cc4e6e85076c081456128a027c4a41389f966b85dd4a139a0cb41752eba453ff4a009f2fea55b7c48aa9054343d0d91c24903ef78c9e29aa4727702f626d0a5fd114fd173d243066df008ca0803ac390b3739b16b435761511002209eea1da8c3bb9a048479e1e561fe3e474eae65699df251b813863922d6cd3698666fa3099257458cdc26709d8dabd5648578c99261ad480ed8c277cc732fc8fb16afde34207f8d58de446c9b4aa2783d030a90fbb9cc20948b26a6b70b9378b0039a69458af2b05214b588703eea9a8986d40166ad6080a284bb3d31d6729781ee930dd7194431e0c892cee615288f6c4898d46565c94e5f6d05746c7ba0d765c023b37a352a85e36a5f3a208c4a9ac0883ce0c9f2afd4c811d9c7a330ba97a5c1329f81838a446f3caca946ac2113481154de3092e3322be44c866b70a02d8b4de1ee472333b5b815d9ec593d0ecb70cd81818bdf21638dddd10214becaeb10980db7c43334cc08fa3a00fca728ae1bc6c2471c043e17363731a2ec9a086514f4c916cde0e4a67239bfd88ba3e5a12edb6ad0a321045bcd921ec781c1cf806b3a4988b5c0a2226fc5311bd844556bfb23327721c4d5ccdbe777207c9787ab235ac6dec0f9c838b0c1d070a2d9dc55ef891b4dd23f2e4d9cd686d5063d7417c6c4e08da053a8ee1e2741b176e8dce061c6b56ba6683797537d79abdfa9d44bd35375970eda69dced00f70f44e5e32e25f6bb9ba1ef34023ce89a5ffa94d69d3e99f4af52f591f003f60450575c857db59ae39d6761f3cff0867dae27f5b9dae3fac1408f6002e98b4adecefd7d46fe7b50d6ac667ecc757f1001d7d98a0a986c57a660c2a1c2cec4d77bb86fcb5e5a8ea9dfaf9ff04df3d335db4a93b4489a6874adaa80da4aa04e0f3b15e28aaacad6814d045fb65f45d84015c7c110d5473aa48aaafdf3e888d4c3a838f283c72af0b7f187597300fbd805bdbba4c921bae70c800a16572d7755caade70e5c463719125f366c3ded600ab082a73d5682bb1a632598881b4bd352ecd57e28855db5cbf60158e13324a864aad8052e047021232d5e58e93e209c7fb2840ed3969fa9e86d04be65fb0426c7a7b7184abde5ae3c317bdbe646b85afb9383cb90f14dc39954c3247ccad383b6e318dc440d184949d4aa7f3a758084fb34c25a1d811a345208f8fddce46c1391fb5d50714674d278c73c9292097316dcfa8acd940909b2949c2a9d839c029beb8db115b82bb9f621580ca80a2960226f55b013294359c6a366e054865c3595e023cab0ef3a357f850162d4e634e371c2e32978b2a36d7422f69ff59a362f842c99de53251d2a40415a47fc489b12111d384af3f7e00106da068a7c00c5685c7f3cef50a1f823e2f6b6b1155ddf23039dbb627271729b7814c90c6ce6feeecdcf8b9b23c20864bf6f4e7fb592e31498c655ce0b5bc8df1640dc2bb1f7f534e60b448b2313bd7f7d97e6c52f2354f83088faac1019a024416e8f8ea18a8699e516f4be2d7dd5a8b65eb7674e5d4ffe73d11b0a78b19d0c7fc4053309f9b20d5f11e4b54c1cf21d9cdacf265bdfeaea80c9b96ff6cb919ae67e25c1f5e84057f4546aa8d32dbe2271551d561ec08f81a302164d54a41d05c10c8093302d11dc0b296cc9fa7d819e711065f7fcbc04d5238b98cb6505786918743ec3ca90974584b4b30158ebf1d5996fef9b085d73c052b57a1bcae58f7497120d0b9ccbf3c24f1660093fe2188c37f83512ada1fc2f5514d3c9f3879ab45012ff52fbbee12ac23e56b74c8ed425f6df665d080c67ab4fed3e8ba0b02d544f46d9905f742d6489224fa1a136018775f533f739a6ddb4d9b1914224809f4b83e4e0734e1e58edbdb7511b5a89f6032e98fce4f1f5e4807b4a0eb5f100a57ebe663479ac4feaac73a1fb1516dca733c203eca57b271d48fd58adf8c731898a7ea857106791f458a9a9e7513ba42b8547f15529d4e6c48fa80b92c833e45922b42af9b1b1f341cf26bf5e5116d748a0b6c4ca752647e9e788ed711772d0659833de11012309adf16f509068348d95a8b16f7677a7ec3b1892744bdf169602ca69d4f57389c8b6e2f10393f48ba9f5277dd68673e1a480038201f14a4afb82a8e873e7c5cee17868b3da169a0e742b263027caefdd4ecf0f88764f8bf4ec0ca210287e9241d6073e50a3bc8e58c38143de367fa5c80b73b79d91d68519228068f0e4fa93298c6a59ff08e9a92cf2273a93bf78419ab7c68d2c12c165fc033d8e1dd682ee31906a75232b7237db9a2d780f8ec7e9290f61cb2aa1f9ff26b8d45a31f1d118c8bd7021f59a3c8337a1f5cda84573cf366d771c65926d752d058405b7d0c7bfe2b2ce7ba9b9c87f13dcadb04b83bdc31cc8b12676aa40c1ef9d2ad55faf968a03a52bccb5bdce2520dc7d8549d4916742292746db42da3c6bdfa36e981fccb6900956222b0d5141646922b06d40b352b364dd1d3aab2346b975147ccbed9d94fcfc69750a0f82ae804b1b4e88518976eb2a74890142f594756a0309f62ee5a65c83c1738806af9fe3a23cd45fb26c9d0272ce2b8b4c57c45f3c9157bba3bb5ff7600743a6b9564d2a49aca65c9ef0be524d93e0f0529980bf1e445c308efb78f36bfb437627224def193e5c7d14f8f52933afc7ff3b2117574f2734418815816118a27b922c734175cc25d031f8a00ed0f43c8fe1cbe15e1901e7e1638e16bd9d90e8c23f9ab8798561ef23682e306e0e7061d93c29348d0a96e2d79852157734ef59e92db14e5bb949b88c89c8fb807d98211b198e4ad5aa5bf0faf1fb950719cef87b032dc456953e4edccc33399e766b3d276c8ab528a35bfcf002a8e48a3e4fd486e0d644f3dcd97624ad7c8aa5b590cb3aed8f1284faf03db17809f4a0a8513e629ce0d575fd9e7f58136aab49ceaffb986c49080e46df3afa9276e6fcffb2cd28b7905b6454fa0d7e60154f436b43219f452a72feda5e1768009a115100e6fe56ab25c8ad0be106937311c1f73a528ff5d614a1c333a80280409f1302ca96a879ec937a3c375ea90485b64008f495aaac4512e0828a9109034cb4e556f5891fce7411ff5b7e56fd7d26e07ade4f8817a8ee5853ab63773dbc20236087f2536c6eb46ba25d5f8ba44175c45182b15c1e4d4ad4b690ec96aa187c29f73720cad4481782c6c6e3005e28ee0070a13d5e2f8db66c5db8acac6e03ea6a5a95e09609a4e39af473bbda353371043ea30df48355717a7dc47391701532606348355eca35f8b90a24fa613da619fedf30c0b55f1cf55df428aea742e6618a475ad9ec75172613c0255dc2841cbd69ff2abf317c0e295a66f4ccbc27406336f5ee9ae5e8cc716339dccf526402713d88d3673602387f80b1ad9a2e6d6244efc9585abfacb3e24dc17268c9368d45569aae0c83e60645e090b8ed99b52b4f1c598de2ca89d7443bc06551a707f2b50a8ca17e9a217377ece848bc944ce2b1fb92ad4bcccd6a509103cc4bf7edd0eace33799e0c2e3d5df7995f0ad07ffedf98784434c99298c8b27b6523da339ccb6b639a12313a770775381ec57479f3f5516a6a7305efc4d41daa984b1d87b33591da89ffd7422b47d51dcbbf6838b9aa728502e6a6ca10ebb0a0bae2bfaa2154d14678239c39fc9992ca52cf78393cd3a4b83f9381160c6b0549241fff2064880a3b909d4ced598e3056b889e3463b3bd4c1909bcb09dea1139c781fe4d7c1b5c609a1232d5c95653590bd9960fd60666b882d28cf8c17dfbb0abc25ab93f4bf59606835aceaac402d2e92c1e497ec9e13697c80b00bf157a68a60bec18db8ab966e51739fe1b9f37a65a6307a0b427792fc36604696df58a7a4eff83fd5e10e70052418378a6a7e3f456fa2703d913d55e4d1387d673a1384ff5bab9de0e884ec6fb97f38581f9198cd4b708bf8fa3f7c59c761f5d05f19f118a8bcf9467d09b09168bdf14bc939293b76d50afe963ba216c0dad869857e4f6fa044387d1e970c35ad5d0979b56c952645e44250e1586b0490b311db423aaa8692a763027741f996f910850086ec8b6945fbbd9b2021a52b79a9e09289264a5d1544c85e01d37252873ce48baf08657fc60a1199024c5741bdbfdd871a5a3bb9ac6fc89fc27b5436749318c74103436763283cfe044bf7869fb480097ba08a64691b9225746e2e9d22d7dca5018813f6a3e4deed0dc7e3ff15ee11fa01a5d5e68e09c2c9770d18f594211be2312dde174e8cb0f1a7253094613bfaddac09123bf3bc3b5bb3568fb18f0a0ae011ff39717e108626d8d14412ed9bcebc4fb20bb6fd07dd867c1b090177c9bbb436b4d0b37e1c283d9b1756cfbb4d869530ade5c2177238508c8ba330e433dd48738cfadd15a8c41e7c971a51be7a4d6c8227769a0a1f8d22fc1159db20aba7cfda25b8fec942e79c8f2bb0bcf50e5ca7276b4e12a27579cc2fb896312e1660eb223f8057cc0a7756596dcb77336703d7a601661a4cb800a9d3eb4583ae71869585c486aa46ae870b6cd160001eac46d688445426698160abe7a1cfd3a356b4f01004fb20928ac676cfbeb37d208cc7601ca06056723924e8f5fc8483269a0ae257e4e61aee4b107b561f2aff8ce6a9a9976352ade270f0ce8b1df14264c6fbd16889a55d5b0c115ef953b98080a0a89c1f616c9975524cb0a7d75d0ef4788d1e88ff7e5db15b2e6c480422738c52cda33b63190e347192519c8d7d4d3c4d6b5f2162e36c851b0615311140b094e1e751f0e4b6bf2c894408e1eed9a04d030a7befd17a04bf469ded52e3be7cc5a022cea3a546b4a6e758d7b34e624de6ece20f68734bb114f15463eb70c0bef47a74c8a1d7cb5664b83388ea0c16818fa4175c0e8ab0d722b9c5bcd5629beb575a8601bd8d54d3f71541d5dfa16ed361b68412fa33e8a5b57e48194475c16e3c63cae99be426d26db7ac4c696e89734c66fda25a27e0841833fe74bf16461e9ec7e44a14e152ae2d3635beef86b735bd7b038bec11d759a9b0f599cd36cb9cdf7a60f939a58ac162804110638f0ecbfb76309a9b6a505c798d52bbccfa1ef4a56670f531a07c96f4d9144057b7c38f2ce2ccd11fa9cf263ae58eb18f807b08dd7ed3c9aacf1f2f6f42113bccdd0fb2abf75cda2562fe63bf5b99b4e0c62ca8d57b101070e5a76235a254f1ca8c5e58a557f7c3afef7c5774f23b3127976f44c51855545dbf7f8ba47754326a10c24be8c1a9d02e9cd58e0990255df9376d6d4ff13f8f9bd5fe96379de18e6362e2960b00b364fecedaf3773e6c9e05343c7c033d64d79dfdad5600d5e42d1e56c9b3e58d4fec120098d673dcfa7e53a3ff7ba88778f9c772922ef0dca3f1bfb6a26cb5ec29dfc1546b5ac3180e41636bc51fe4759743ccad17482bc7083ca1acb8f461f4425dab49c360059267eedbed9bdf5a282b43800c0780c68b7333f93886d18ce6859855cf1e13f5b9434ab8d986cad87b0ff61e1d315bd2d6f19dbc8669f466af6133af0c8912c6638e84705b0ba2c95c6027f323d223593eeca42801997862773ec8ae195cc243d76146ce69440f4a9ea202c3ce0a2a5f27d3602846552b3b8122dfb665052b0fa0d89ccb02a504ed00cf26d6e53b44906b25f385931f9414ec0550bb1d2a2c90f208ff7bc79f5d21f899d3a6eebac4c7cd63d06fac23d3e746f9194c93c24e893599d9d84e4fb2d7916ec139326776976ce5f870bb1e46794386c22ebf98f54eefc802efd9e6021c7387ea1384660c4be26ad90d26b4715e0b84f2ba2d2dbc6fad46455218b87ab9b3fea616b9eb275889cd73522280886c4d4f7c11fe981afaec605c5a9d11deac0968bfee339297afc87d3ee41e7f67ab26069c0ef00207bb0f788764652e33693c3250f61db3e084fde9cd288950e01e4ea8488cd29c258f29441ec3d5ecf06ace4adc815eda47307bdc87d134af170861d499823a6859010e21a2d63621c63f6b0318c0a28a124c02d6d91eb04f6421c88536559eabd20c3eab5ce2c821984cf4faaf34d14a8f5d8afa90b85a7625389ba35450d781d5347e38711a2b52f58875c78d8061511d891f12ac91dce515ae4dbb28de17e3216e57f819a36a4f39a8efc5d7cc242e5028c07b205c6244ad09cd4e5d3b11d88803b300838690fcd5f7f86d01e865585e37ae64092e82cd7a968d98c82d9f85fe0c78efa6abb7ce9023a2f9c9760413e0033f03576f8350a031282ba42056b5ff73cd7d93691f03bca6e18d2c092077478608434b0b8da2b957be6612ad780abb54512be17ff8ee9a7b44fd02af63b4da511de9b715c81edca9e3b89c0e61cb46b973ad097e42de070af5b34a15aa0e9cbd77f5b2b4042b759bfbf1fe31042f22fd0852eaef526b24664c2e181d39e9d63c0975d0a345e99132a79d8d8f16877e05ea63f76de77fd4b10982473cfd26b7c27f4ae506f48457f6f14dd401a1c92866886ac2c603be9367a0f6c942fa6c22565c12187f90d45bf318228d3646690740fd9f07d9b024225d357f9e5bce77791497e14802b9f430e69e0b1b9d54f09ca0c9c3f8078dbc1c244574365e977b1f475f8204cdb2cd25470fb79abc60ad7aad4b76f2ae574262abb56998b2e76bda25f79ad9da443b22681a862f1a1ef8a9bcd546faa776010928bd4dca4b5e216c0f43a7b027816eba846012c5e772718184a60dd6ec88324f11782054d6e36f04996f2ab3f648a237bc4114b91bc858191b1ac7106ce8709f62fbd7cb430f2bdfbfcea319dc697830847e10f864a488ee10c13d47e3da7b642ca9b8ecc36c1741f8ca7b34db2588d96fac47d400a0f373b62d33fb34078e5729a5b077ace0b833f5383f7250a6c2076b3068e1e6a89cf8dbc9354f518123b2da5554fc20533dd2672c4e406610c2cea78bf9935156e75e9aed6d26cd3893f8741cde83f8f6346767cecb92ecd650c64b5579ed78974e7971e4d7196271bc8d8d69f27879ec1b29eebfde9996ac053ccc4d09d5b01b2d871f01ffe66989f26e96bd299531e6bb7c5a978efd8e0e0fa418fcca2cdcc78d032fd753c9a3c56fc14e4bff59a8fb0796bba2931df0ced6c434102409f156eadf392b8abcec4234c610c53ebd0c6a2fe3dc60da87b17889b9896e7d1e08d1b320b0d8adc89ef86f75754ac637bcbfa091815fdee90336977b1b8b761f399efec378c4912a2fc3f5b0fe61c9b68c84652430c08edb1c6d2987a5ee5175f5d78298c4fd2c843f0215d40a22d7f856774f049dec182a96548dbbbc3c090e7efb5e0c39cb32343491cdd8ff9f8556d5391e80dd136aa905ced6af9dc94ec8457addc97bfa20387b28bf76a85fbc1525b4b1376be584b889605915c4e7d63da3ca0bc94c9c0dc51d8a72de4ff2faf5692e29f484be1bb7e463098343411641d5dbd193d8bef6074be96c58fcaa8d1084c877fb549cf8c4d3a22cf742553368aa22afde60198d5adbed9abd15983de1b1022794b2175ae6be11bfc819e48a50ecf994c7314ddbcdd68851b1326b49b093c4d96a9bce918480d6eab7403ac779c037b7710cb99f5d874ac78f5ef302c280d57927e51477dbf0608c77c81b7adcd05ff1532f1230b9745c6c007b4e5fb6ed1aee0e59903f26ecb9f399dd3a958050e07dc99542734d142da4a7f8558bfdd73840d18baeca2483e2dfbd6f11ce0a43102dea28f377faf1fcffeb25c6ec745b87869f4d0914e35f8a00bcaa761e46239416d0edb31811051a6957ac940f31a63805fc5f5b820de366c5fb7a3dff38a522b626e1a045a2115fcc89a7f0131d3d6aad2d33d4fd6dd8ccd295acddfa4e298411533ad689ce408d085a43aac9a1e4580eb9a7600718f6909ab4e00648339ce971290d5a18757451edc5ab71794b2d18aae8803534f3c9191f996feb6c6f1b95578424de93d8bf78581ddcf030de55346bb687143ab218e092b079967f130682c62b70887c7191aa6baa471381d82af6ce5678897eecab7e03ed8cc1afc67219f7ddbe83210d015e49e62ecdaa0b15c0c90b6778fddeb0a698fad0a5c60dc0013901e5350e3c10ff4a542d07f636f3917881725519db5c866444d7dfb1519d7e48bb32b1455c5c60a751922a01b73038561ffbde995ab4600dda8623c561664d25607fd727f69ebbb5fa692fd30cec313b10935a684e073536dd9dfae6e6ceaad249804d5a14defd1d269c44f91bf36086c22871e3d5b5d4dfd19ace6b6e710d924c57d67ac196b3d5b6e638cc86409ea27bd1a2701089406513713641e07f3d4ad5d3857d37e81b545b03481c807030386ba5bceb73e79a485a8e1b9063d84221baeabc4049509406f0fd7b9ea11a469bac8492b8f9a02732a77aa69803a50dae9f1b9db2e20712caf3a4cd76ff8c4baf8dbbb68df9baff71c291ed938d100ae4d4a21e3eb0d2dc490b1bbeae1d63c17833cf05e17a5daf138924041d33055ceb40d6cd0525c5d0c1955a16081cf3c4b07c06f6762c4a19218617dd56a4db2f914ab6efe6ceb9b5d2bc5fb66066312e7b58eb4e31bb1d78cb49cfb9532c800278ffcfdbcfb0b219ef67d1d66beed47d5e777d3646c8bad2ebf0d052600e1bf10f1680eedff87317cf7f8f772828b624d4764e7499361677d6e5c3914c1292b15a24fc811b9bdfd63fb2c9c43118e2cea256e6dc6cd470bd822b126541ccfcec108306a619644fc976bc360c6594a45f8e471ae238d1bf7f248e2e4429ed8522596f92042e20c298de2fcefb6ec0b063a4ab0da87adf87d3819b8ba3df82f280002e6da32a356060a63bca123c0bff700dd1f2a947d2718d51b6dd441002f1a5c8b52fc0ba2c607517a16aab04752f2e84be135e90c5252a21068dee8dd4035ebe32e9a1d3b9d531826331c7a9940a5835c566f79fb4fd9a0a99d621107cf9bbfbc748c316bf26adb4118dba789d2ecc37bae39abba667b55392402705f0b0acdd7577c1caaad7ce44d16eb4d68bb0625be63dd6dd5cacc39a5bd8daa442ed5d49f39d246da7fe4a33261aedf3313b9005a1928a4204e7b13bdaf5525d93d1598674f2172704cadd1faa7580c55648a3a1607f250fd18289bd0c56ac38d6c5d29dc8dbfb3b1c3e8a830a0513b964775fc022e4c4226eff36a1890addef17da63b23c95ab75e9530725e5e2d091cd401d1e42e867376458e17da9459340c9ffcf1a929d5b2665e3eb8b1f6241a730e742b2807bf7f570127b93724719ec47a612334610cebeb284ad6ee3d148eb2e5f09dcbf96cbb3477a18179d9b37a31b6a826b6034f691fd44b09bd1677bc186e34a2430e9559f710bebebc8c6cbb19fdee0e91eca0b46bac909298bca7306a91194a86547f81d2e12e37ad00f5bb9211e9aa682c0647fc38ea311877e99a46a3453590beb37959ca59649a7077ea475ad886ff55cfebfe9384370659af32d214618739136236f5410a00f7fa31150ec7dafc7baf3c691ed177cea80aa667c09ace5fd20da22e237204c7274a0d18ccf53cf6f063486e3b1ddcc785c721c2c215700bb84f4e77adabd83355fe216c3d3e1d316e1b7aca2a23aa6aad3cd1ecdcf778ca74a8cc7dfd6bca6da82a47cdc390a1eb9f1104070e900b8b0e465d3008bbe35fb9422cd25aef4ff342699f4e7954abeb13d7e2913d3a193a181c01a7ae99dce24bfb0fbbc4b9c5bbdc1113c17da1916811147f61d503f41e8f59d2b497d18ed2f0062e45e5e980d293e734737ad07b9a6325b964b17d9e403c9d5cdcd1eaf77937d80a29e8acad8989c6c4bef3e4d8230e46b1e985717654e4621a8fc2eb2acf7465e78c8078952561b70ff501bad3a67ee1dc4fbff53f1b097453d9285265b2b726afb16afec88fe80118be5ad90b84e0db2ed18c6d7309ae3dee971053391fcd18d6d63f80050cc8c0a6cd4e28bae18c3b37a2aded56b5fe44cc0a0a97b157b4862c32687d44fd2146548e44342b41df6ca8e9706d72372161e7cd85ef366ec2dc0cabeb79a7ff521550f13de241b22309da9f9a316f8231a11691f2b3b929fc974c750fc45735fc6bd3858cbd5a0853706c1f4ce59dff1f98d0fb790843e15ecc66c3d939a93429f9d973205f720d30cf5b025ced823782ebffa19c6bb5752b4f9770519252f5b11bd09bb6e7762abf82cab7eb346497e5dc1ac74196a42a81b6f7439b5d475ae6b9aae9290741b3fa2db8de600ed73e04ad8f8506f38b064f655086da936f2082ba6683e170b8bfabe64c152284ec26613da1d02c14327451b10c2ba6c4766d8a6f5da884900641454970f850d1cc21503d915bbe4afbbfcff67c4f44328531cc29cf5dc76ed48963aa0a959b8841ee59cc5114006559a9a06db264d1f4f94804886cf09e42adbcb82f2a41b7238874e70bf064252efc0a39f0265fb761b077a001a2c821189458a432eb73cf937195109247c87af71fe30bca16388929c426e7680fd71cc49abf39af46e08cd2716bc9f2d4c8051fc992a38bd88da2861727adc2ce40b825f5ba77eb599abb14a78402f9387004a1e2a8d7f4671215f3943e295de2769a82729ddc160d70fd3cb043f10fff46401fd10e6ed3333a656cc9f82276759f0ce1ce97f6656547d440b45f71f90568a8b8e917983670c88f53f2423989288201d7d73370df56e23f8db71cf86e6f89e6920dfe52a64840d414beac83834332c29e0f90fbb59506ebc5c4e300c66fc5762fffde511503407ea3faed99e891a7a01127417555620a35ef583b37cb4ba78e9c7fa6d0c1eb1ba4a11db3078346fd0b73d75a086fc3d573ea35d88187846ff7beaf26aec275ba6ed746f2749b74f1d85ec962751dbebfe14a522fc0dc1b625259815e95d196b16b421e617dc1d77fbb22a4d5c111967e439bc1f2d50787733e4b508b79168606226a1e3717356da8f76a69afd7db947dd2e34023d93b429fadcbe69cb4bce979cdccf7e78843c272290c61f561203d16811216fd8f35f1b096098c4eb4a634c15f78e84849c8c4293095ac7c2238b58dc3e00a2039dab7144bc4f57bb24bb5bc1c4b484c0bec2ea2af62d013f4afb24fe6a6c87f5c321c05184bc7e2282d0c7fcf6384bb40246e2994826db95f398b000f043dc70e4b9f84dcfd89a3199216b3014eedbe2b0e26a82f06667304b1235bdc72c1b5840009ac75bdef94b7bc56d21c68f15116ee895f760b4c394e553def909edcc295e10b1d629a414ab5f96c46d8513b1446ba1b64330ed9c5bc79f7465840fcaa621aaba04c0806cb035c0164f55fb6ebb4473383963ebce2dfc8a1e336f5d9b05696840ed4930acdf6b39aef98ea540fbe725f0fdc093364c19da7bcaf997c5a5fa95a311075c89bf7457dfcb0db6ceb458b546f3c9b3d7d59785a6a669bbb41df44519f6df5d04ea18a04b2685583c421ffca0a5c3204a293337a2e9db8f6d7b363f4fd1c87948f29f1af8c60f70e9eea169cfc35470ec6997a653622d52e77729bd74b20dfe12cc5fd84eeac1f2fd3946d6ec9cb3dd5541f20697868c5c6d74051f72b232ef76acee1ef703202140d9d1a5a3e6cf5b88a19686af3107c1afd5f25b2b1c23198839b9e7accfc667dae24e3975cb8a54d55776f31525d722ddbe199021cb66fbe8c5580fbef44c58d18db4cfcad574cc30b3d10a8863b64f1d4d7f494a6a923cf577755cbbe44ef8ebbc8f4eb90b903399834897d60a8baf326b70a0b1f320782fedb5cdeccd06888e0145c669bfc5e81a21916f411f60fe17b150795fd8c0844d582e0bbf874bfc168f7d4ef8a1ad27c4ad006133defc2b5c9cf88dd8295968ba108f1b198500ec6d203e0de358eca83415150c44524e47da2209bb26c932ea1d827236802c6491137164e2d7e7000a52b546f80216defb16daaf7b2dd8e5024bebb5ecdb7e2b0f1fd3c22c6b755e5e8a349b497501f347f99cadcd8933f414501da8c87775aabf8780416368c092c835fcc64f7ef240dbf4140537048f49fc23d071816dc60d41a41f0b6fceedca4d22c5f18ed8e947fc31886a4512652a0459a1563e0bc834c2eb4e914419da603bbaf94e45d9dcfee20c71ce00e461a50bd9bd8770507ec04b482946833e600467026093bb4cfaaad7c9c15607fc3eb2f0d7be3923a7e1d00335644243807d2d892f20c6ac06a6d6def29eed6e79f359845e731564f44064dda2d4240e40a40f36d7b0f8c1232a24855e149e9c2ea50e426619a83b1c5029fd0eb4a2a69839ec7287b7fa97b45d916e0257921533a1890912022d06f357d017a0c13e124908468f044602742b32e14f5577f698f12b7e2399e7e7070321a7893fe7951dcfd8386e50033610cb989336c16e05c83b64e79d8db52a1419bd2af32cf7abb970ec793ff4d95dc7147256dd6c63aa26bc43ae425a2ef463f7b24f1cf6391a9618a7f4238f1f0d08d92293ca3a8ea8b56277cb22c80ef9252296d12d943e32321e9b0b5644b32fb686c70a68887ec37e0a0d68c2dd1f874f83cd9b51430badda0490365e3723d216fa827f23462656a438dcb426b8f1796966456251805793aa57b5ae45a8c24116c6dd75c1ded55699871c777e9343be3013921a787e8a139b2b110fc95c15e93d433d82b4f6b70ad9c9a907f0c90b8b43bcb749c8457bb0b765b00c2f1e393334f675ab991946162d165b1608863ae35fe2cb43377e4ef01f60fb3142033183769546ce766cd1d1036f7a357293c2056d15bcc3f007f9eebf1f95369094343863d8e4fa5369a5751aa82468c4e71817f6d4d476384f60a54201e830388b8ff521926bea262a0096a22920a4b9140635289cb135264a7ed99c545e53bc5405f9c7f678bd7a76dccbc1f0ba4d21e24fd695cf808b8453ec95893646c2d31f9101f02226ad1801aee467f05cde79ec5ec125a7a6d6c8cb1a0d673c6e58060f43c0b7414c1e3992f42f0c43e01f8f93c9d6fdce253095d39c67f35ab67c56d534a3547f64d60d20735335cd29d08e3b8afd3bf0bc4ffa5832abfd18ac039cbe9e3d22275592f11998f89b17c050ed80d392c7107f011f78ee2e9acaf988cc81ca474ffe1bf85e428ee68afb27e7ad42715512bb6a3129d0b5ce6ae0629fa348464c6d10963511b9e9d6570b11a31f6e8e1c5fdc01167799a7ed9762f1c887d1f13c29c95f0b1e384dbcc196130aa096bc643be22bb0c1d47404a85dea2e56594f55fb41e46e1d5fc8131049c6bfa68a1e206743c5c23b71976c0c6e15bc4ee5323149a75e35362e58708b603d8ee30c62c83ae9d843fc14218e14fb7c1150c266af1f5f1c5efeb19a35df6a337d2e6c8b690436b78b0358fbee2dcdcf7590fcfe8deef7584f8e068baae1e31f265e4d562798793d3e14495f0983b5189a000711655bfc79360ca458767ea5143f99fc19b1a2ac4221a7b0a004fb567a6cc625fa75ff2b7f8145ec155073c109a126ac9d98578cb9e97fd4f387d6e99e8e3104c0850ff3d6be24696b044c62e27716c940fe83b5091394ff8e427850470c684a7d5d5d34bcf8bc60ddad26c24fd1e2b2fea2ac8ec641144aee674b3d6b241986f834195eaf9e50794721d367e5b76ffc1d90c7bf8906681ef996bf5cf576addf7723e380db8714e817e5022dff397fb361d826cf9fc083f0d687132ddd3d94b37301533c5ad449542ad2f090609dd7a4c78ff46808eadce7d2c88682d87d4309b534ea12b5ad7a421732b12e88e94b9795044d789948f3270ed25af30e8650561a864ca2b9a93c19e7465b603a4f375bf48d6a5c2ec0de8e5518ac649c8f399e734ce0abb84ffe5f52e4dc88c9a97eee1a0e3b622459b0f3f162a01a845ead8be31bd623055db6b862fc41e56f587edae47e16fe0b6e2c7916b47ed4ead5d2d46dc9a7b818adff2642cc3c63b5dbeb2bfa82c52b3bafc236b7da577473bdaf363d190bd86bf04b3fe51a0df30fcc00f1ceae9e6abd06e8cce40f64b85e393a24a4fdf9af9a016114a378a26337c33f7bfa1f698062eb6888072cb5067f852a2f41f12fbd4cc531ae49ed21757958b97c785c8ae4fb896f40060b82800100dad7e2c52f693ee3c931c0e6bc01e15a1504303215ab5ecce4e94905633b257615ecb3407256a4887f5846f00a111765c9ae1d17ba3f1ee6afc5edd8422195bc10b92e5e9bce0cefb7d8c5bbd6a72b3cfb6ddc34b54d190ce070a2302b833ba14b6483ae056f21efcb4fc1f3e295a2ce85ead4625805d58115cbde8953c6ae37cff82aa8d7c534781b206dfea6ac0f0d46265459da804dd84f0315b1b573fcbfcece43c1261d03470bac6d60cbb6cff5d24fc788eb21f5aa1ae844b55146563b6f40696ac91ea23062dc67ce2c3a296efb9bfbc94a9b61eca062fee994a97687293fc18901bdd49121df5fa43550f18f4e44bd0cb89a69abbc2f92087d17edbc02d310b31df24b618f1a9b2fe7f22709ddebc3cb35fd8fdc2a5953df77ba7c4c205802f081e7272ddaaa2ad20cdac2700acdcb5e471d9302f0e82165ff99ca4d55d72609bb5c3adc46fb24236d1f6aeb3cdf17a07547f92884b31b907e6412711d5b46568633806dc5056effc0882abaada68b209a401575b191e3e917c04db805c270aec963c7708029df57817ee594adae3d0aff1a9c27db0d30f135d4627fdd3f00c3a0bd6f945d239fc53c60b169182ac9d56a3afdd65780e0f1ce3094e9b9589b91c63d0c3566f62e5dfac4d62507c933dcde07055878e834e981879e70e09c05412ed1ab87eec8560d9bfabd8ca6e59d0446461cceb738721c0863de0a57db6922b229a1fcd01c0498f75650e56e43b144fd2818f5ca9334c248d54b69e88e51bf93ff12d072f46fa681d6900fefd429aea2d8cb1c36796f2df6dc969245813ed929a27de2a6839d0fbb1e057dbf30478dedff95415c75def2c4b676c3b2a86ba6d553a7f636adbf9be6f3760212d6fbf74fb6b85d07e27c62bdb3cd1bcd9d40ad00b62aa1ce0e303c5aa9a010d057d51f207049334a8befabadb6adcbfdddf0dfc9721bef2172987cd697076fe1ac7c909ada502a94867c4f908c38bad12a5b96615e82b9ad58e5e129885cc1b4282dc4c23bf7a056064de4a86a278ea5a46a86889116d7079242010ff67deae78d5827ae18e2eb42774d60da58cda14571c68672d3408b3bf93f4ba84db50aafa6dc7f336d0d8f80b2e3aeed9d340a764935c3835f00bc1acc89a161b21fa750e4c919ec3d602b6bfaed29ede346b089203ee9dc855d12fc43d3649f85e2b776dd2709785aa6f71062cec0defae9e90a16aeaceecc2482bb996cb037a3495e5e9f108c35b2ab0bc122534b2677dee21bad5b3ef8ac867fbd7989df09212250ad7ccf3d986b39cbf2216a7f8254baaf54129e64e60100919e458bbd69b75808e1d764036adafcebf255ad3acc750f088e234d02c5146e259874a53ea8d9a38f0c33c98d4b5e236a9e07fded74814d0685de09eebf3fbc0df02ccc18cd29dce0ccaeca756c74c4de9d88a7d4aae796f9231806b716c11a8ae651c9524c8053557810b4c2bea8b120926bc32f29a050be65739accb8424c8350d04c0eda5f8c0fa209e8f7187bf284f4c4e3c8e85734c4c1c503f108da31d3fe9a03d66149ba4abfdb8247953f03529e5b22de689a249550019eb03e47e898c7948defcb13464ea0ee43c0e24fcd38afbefb038c082e037d72df96cfe40161e368ed8766bb26ceec90d1bc204b792a6367cd87cec377ddcfef3b529be5bd00b9995aaf28504e6c9ce514e9676814bdacc53552296f53122ddf9f97c9e73a9fe13052e98391109d84687e2993d9f9a68d3da6b235014b2efaa89efaec629651b8fb2e84626c9368a80bcf59589ac963b0847f13246f909d9bf26ceed3842ad8f34dceac62a8d0120f1ba1a8fb46291f7259a67a247ee1286ae65f738108a555d721671100119423a58b33ddcd787abed9c97574798b08e4eb93039843b4f573e59019a3f1ef87428cb1b8a037b223eb2f7945a0ea0fe9a5d1a207a6786e5ee40ae8874144340b10dbead5472521afc3d2de9ed7cccf116d9bf66699c58739773aa16de134ba7715e47a25f74aa45450fd813650f5a71a893abe0fa41d12e43911d9d33594f2a768664e40dbcc8ac3b85a9b1d9e299f6125cf5e4c3c283fb95c051e5cb2c6372eb3627154926159fda835500089e119b095673ec96e658bc21285744023aa79b83ea09d9ec0c6b110f3cb9b201971b9c61771e3bc99af818069a97e8c1f48f82f90c6b0360c1009acf7dd6102b2eb63c34b7227b0d294e5adcdbc40447d2d26cc8a56cc47ae9706a78a88f44da01d578f266d54eaf41ae6fc1497e2dcfe5bd35b0bc6151337017509774be4b90809c4cb237459f3ffdc2f39e8284692d5f0dec99087843f2a93b7587ae1938d55a815913e8da8985837994f9f524de373c3510218fda2f032cad4804d7b76e4787ba90fd17270a13582e74994b7d9b9c92e16d61f23fb266d0de38102cea58b6fd6eb95c586580d6dfa2ef83ed78e49401e7e84a65254b230356d6c66fc8d06941e5b5f85977ae670c7884543612c8db131ed3a7e61e4759facf359ff6f9233edf224c8303f1cfa18677456a4c69ddccfb4a8b17c54dd6afe0a3d335017a58298181272405fd15817c13c40810d4b7ed445860178dee0362a71f933e82033f4ab37d4b30478c549a7b5d71ff6495669a1a760b41577dce195a89f40e1e9d43eefb9eead62473e1f2ce57992449a6742c4936d43282c7614c3f5863f2d63c8c383f19579c33eb461a4d87aec83cae47ee55628323aecf46c2c90c3b5fb624de784f6f8abeb3f74f5be3c9483c8861973b85652a0e79921eef4e3a6085a4bd73035d4d4f3dea8237fe92152ec616de632eccbd863e18c3813cf559d7665ac7a4e5239d3e19bb2ee1890330358cec0dfb53cb958f06102d3b5004ddeeb571c2e840746cbac64e8ed7715c39b1999a96c2d80caf48ded607274fe9aded6814cc9f1bfa47f98cfb91b440772b267ff7e9afbc5e1a529b7a531bfa8512b9ecb31555b9ff124e71a21ee25a82643a53f16ed7fdcd8c3eeb2546b08627b800eef434b0a82386304daef4f2d6aced68af63f911c7f71faad3705688d8df981b320548482bd04c5ce5eb45639e9f2b963623f998081531f98312ed15117b6d8eeaf8a17c085101a9797d0be008b7627646a069802802a2d1e5ca4a139538675c4dd26e01bb08883b9ec13fbb5d7fdcb3a3160135f2fa5e497289a5590c01542c22f6490f759c8cd153bd7f4267286cf7a2903617ff7e3673f90d2409d7b62eda8e85071c7329efdbae783cd4a29a4ccbfe543146d354e4cc0e1178c8bf335500b76cdb42e874a93db45a85fafa298d2a30fe2b0744e013464bd1b04cbe79eaae4dd30a245aeffbcb6fafe853d0648c867164c43875e9c63f5d0f35d81f938b4e2bb08e991e5617d9e01b4302dcf2c99f7d325971e3e9f7adeac133cd3e8edd9c6b0ec238e587dad74f5530703f9c99c12e9ef7d75e8ac8132de44489a01b850f1056f35c5b1ef6254005e8ea1e80a14cf95ade507681fde2880d97ddc6d282c7623c4f5a5dbc907056f411e84273d71b4b42e633de18e0c3088cc48913f0b1654298be759470b0133296f7a41f9f29d40fd1d29b6f8114b1c7151e4a64561d9ae1cadbf358bb15af4ff7195aa9d5560abbdaac382480ca375d728381cfe4e2acf121fd2a97c9e40a56a4e0359cb2ec03df1d72a8f156379032aa5bc90e39a66083ed82001906ace7bfac01394c8ced23428b50608c95c1490038b995dd933911ee989dd5a63df504925dda036c22a830ab275eb28f84c5bbd02acb4bfbee62f1b4c34057d67f96e85afcaa0dc2011d0dfc2d18e85c57bf29458fe4089a86fa8bab3a5b9bee181bad0a2ae394abf9152c62d88608fe4f6e2099930f0d7402fd65e3c5894f68a8cbd346f4cabf00f6378cc898756b5aa5d26c7f4edff7e473c4f32281ccbb319e3104eb38d46bec19889fb99c4c464a1ea2f3956852987f4b71feb05ba807b9d5a76c6d0df1050875b3b57c593bdd463b4da1a6387589cd2debc3846fa81ff09c7037d0fccf01ceab2a280c3bf317dd06318ac1864c60c6f392c0391eed1009863f0d27569a5705f6a8d09fbddc3266531350c40125e7437a17e17db24cf6f0638f1e25957d35426559899a29bad4f860f1620583d896c1a3a79c985944e411c86ef1ad806087c41083c08453321e720f8baaf4f258beeb4baef2677bbabb71345c0eb7e07b703cdda8c90c6d9f6a81760c570b57f2c004e15e641515823b2d900524c1e38c863b919cbdac5c678268e5e105df110e4678a0912f3d2240d8cbd1182e9bc6d5c076f1802254391aeca360fa03d48a92040036b4212b3653b3659905ed5c4dc40ed4ce4ef5804a4b784f4927408325fce14af3d45d0f0ea55a949fc06e71b82a602aa90f68062a853313b5f5edb9d515daf33e1e206102d55510162de45f0e0b7a1d60ded788bbbc3af9ee9fb187d61288f2c19f6a1c04394e0b0ac1ae9f07b762c736da22cb5ba12ae8a9f099aefb1d8fb6921c4a5f3191e60e9bac24c696644ed7019a0423444710c523fab7396952addff94278ac1f6445fafd4dfe15acae853a736126f091495c463a6f21688513a153a5b0352297030ab2695c0c0fc74c9faab5d9f994d5b9900355530efc8081eaea74cbc9ca285d910476991c3becb2d003b67dd1bbea37b28ba9053589836ada35f3104a90b4cc0bb54ae51ce368a8f92a27e11850c0e3da5ee1777cca60353e1eb4c74856deee51f3552653ca4aeede464b34e722608adc7f51aa13c0baa8fb3028171232bb7a0b27bec7e74f2720234e4cb98bfcdb5e23dd32a3b63e8a5743c1eda343e91aa79742a6c7ddaff54a121eb1822bdc8ab3869c4f4c0a5329a3c6da0f72fab9799fa81569574fe7d1e0ba6098da0d7ef3a4eed6c259ff3514e310d8fd760a32a16b820c496340bcee9715f623ee6d26c65ac82e9292245679def436349a5f0ff2e78a76d7dd161dde2787cd868298513f58b7546b38632351d9c240812efb0643a11d24a1609fb9c6af7560c84d1d331072fce5e19d67109220c9ee8cf94c2eadbd3280a56d5684314e95d7fe408ce6bb5650ddbcb5825d8c2479a369d065e5d94fb725bac5af0199f46391015e41b647221ba439bce497673ea6b305fe8bcb7d43f8aee8418545f0eec246d1b014a5f44bce1c104cb7f2bc5d282b723f2a3003e23d0439fa17122260173219087952f900677cff70f96d85438524596681fdc046674ef275c11a8349dce8d4c6d3c42fd0ed4bb50f52e2415dce4b2e0f8f3f0185d86167405056ae03d76ccec740e2e659d22abc8bec28749f54e4f1c05522b7a8a3f58068c101b0052b4efa77f5a1559a310e6c876999a53b107292b33712ee02999a22269a273938980543be791c649cf89da2b58c3a39b9712c3d0f2b98684a8b5e7a8f82b98d4f84227a73e73d97d7bb7736d1801acb9df86b02c0beb6331245f6ce73e11e86cb1dc38b158bbc2742a958fa412cb26fccc29bfa5c53721606b85f0efb88b4b73151708d3051f010e8c2297ba8c4b5053306b22cfadc31d692775d0bc5a1f548a3b497adea49279844977744ee735609266988b680ffae56b98f97149da659bcc08a6cf8bff8257ace00703655620afd32879de87c2acc8b8ec2bcbd3ae352d0ea64163b4f1500cbd68c4b3f58d2900c645fbea6bb086b737472fd7e04ca72339f9c8d69f48484e833b89121f266f13333ca38d099134b0b158154461ffffec970b298f510a0492415966eebb2bbae1044e01a3cb3e2614b69ee94fb05409f45ef5bb215afba6414be6d5d69da4bde687896e5d3d2876fd45b0b194506bcfb831214b1e254cc6ff71761eb45f6b3eb2fa33f354a20ca54071f37698ed3df90dc4c34d7dcf3dcc8ad49552c2606e09dfcee7577b074cbf455acf461b16bc8e9fafe6178b78382d8f5b6507144b1b2d1f5a513494ef7143b0484d3112fdecf7ea79d8904d9642c46315f3740fd3c62be7968bb6b2d93ea0dd57d30c4ce75c8a32eca486edad91aee2063db73e73af11892c973ca11ff72cdc1327fdb322ae6d401de2c0be0fb81ac0c4ca312876ca5f5b2f00ac85213867e0e8f6f14b4a974eea5e9471d1cd4d9272412838ad6e34d58c4bfc57aa2273d64d599fd88fed81bd221344ea4d0435e4aa4687093984573d3c587af16d31a3dd6938f38e13513c423baa36d3df4edf5644712165dfb674ae3fcd329cb982a71e734c83118b60c5425ee7d13f03fe9f8f56863f4fbc4ad15e14c4e1807c9dcaec30b751fad5ecc2fea7c55845c0a57c90642fe6d42b28d5b015bc82e07732200a50d3e8aaf5b427f1734fe0c410a383d1651c3a6481c660557bf13c4dac94780252a72e6ba2290c058ccd03e16ec604ee66f100d4a95aa19f099dffc2b8624b814fd5dfd305a61d789dcbadea3c913b38c9fb79748b4f96271c66395e36dacfc64f349e3bb4ff4a7d3ad3c237")); // 16384 bytes
    assertEqual(ev.size(), CloudEvent::MAX_SIZE);
    ev.contentType(ContentType::BINARY);
    Particle.publish(ev);
    assertTrue(checkStatus(ev, CloudEvent::SENDING));
    assertTrue(waitStatus(ev, CloudEvent::SENT, 60000));
}

test(08_publish_structured_event) {
    CloudEvent ev;
    ev.name("abc");
    ev.data(VariantMap{ { "abc", 123 }, { "def", 456 } });
    Particle.publish(ev);
    assertTrue(checkStatus(ev, CloudEvent::SENDING));
    assertTrue(waitStatus(ev, CloudEvent::SENT, 15000));
}

test(09_publish_multiple_large_events) {
    Vector<CloudEvent> events;
    events.append(CloudEvent().name(String::format("abc_1"))
            .data(Buffer::fromHex("cc175a067eee6bd20af0b94849a81a628614b87d9a4b6a7ae64afceb66e75918f7b328595de4d6c3634e88f0bf1f2d610b2e559d0b9cfbc9ca9eec58ba4bc98909385ac4e7daf3287d8d558ef47499099e9e7d39240f8b170277c77aaa6a4bc0a1bbba75a16d1578e366712d75c851b4ef7a5e2f63bc179c5cf297e188f3e159d6f57f6a700c47a00e3ace580487a38fc3056a4470ea64ec993c0247d8385ae2323c08ebc77541c7813249be2be39fc5e7d94e1e10cd5a3037aed8c79e604f7dddc7e27ca25a2e62a23d55bdfbd4a26931f4de9dc866a3c1e81ca38dbe5406005a314984aa9461ca6b505f2e60f926e7acdaea9fbe507f9b9fe1bdabd335578c0a962ac895707e6a11823fb9dc1eccb23aef1fea3e99f35a4da4e0a0a13f1c6f17a74b1ee9b239eeba7887c3cb29512f2da6572c9f0cd1f6fffa3862c6df99aeb89a270ff3e69d3cb0df3c446a2e64a815fdb9254d02a0ceaa22b08d7b3b72404be40c5fa4a0655e3d70ea08cb8ea000c81594b0a5c36d02816833a46129ec0668fba21c45f64d258b4af831c86887b1a6e876d9b34e96184fe620818a7a3fc8968c0e6ee052413fd53a122aaf9a58c0337ed16a639fbceee75335e6d4307265ed03e10b46487a592c55aaa94d4f88b75ab34d4c8412696238141f31376500a52c66031d9039401392f0f4a89e287aa11de9674791f1e1d9b2d29ee19db12dd3b16749be7add957911d7d9a68d17709ac60017b7e7923779b6136aa357bd4525e827cd5a156e70ea4864eba6a12e0eef5611ed74df199ddc83bbecc77708725b522ee01c736d4dabe4df3572890360e2b399bf424dede8ba3d3d9c295bf8c700c3c0ce1f71afd2897c8fe2d1d949c4827014cb00296ca3914a07c858e47e38236ae77effcef4c358fedd2e771919d7f6f657eb4a243b9360dd81a2926c7355f03b12ab700f0b59870439a55519537f9ab3610fdc5da56c5afe158fd5199bc9985051b5e4d1c4da3feaeb1bcfdcf7fd00f976e224e4d4660e9221a88b31787d7207187daa05590e7632b8091746c2e530e99e19df84cdba0ac8643a4d374713699bd517df79de997789f9c758318c5605360042ce4f5d70dff39253f8d7bf750c43ef823de3b8254b5d134c1d60c94a0ff772e04ccfe56c6915015cdae396cabccbd70285432772f40e0eabdf56f15911157b3adcddcfd4f212ce997a963582e52430d9f0019299b1ec14dd731a6a406b33f9dd5e63b3e02f026cbcfc9385d3388ffeddc0c856ad3ec8746f8d0ba5d67ee5aff0072b51a9793d75fcb8ba0e223524af1ae8ae4a2c7cf7f5646666e92c6793b29549c3ba6ca422453dfea2d4f8deafbb38f2bef0b16538487e2f7fb8b39ad0e5eaf2e39dd4c77b67b5f35fe908759b7eee38359a2ada1b689b3ae7190109186dcbdadd3de6a5bba5f3e9f863be40171a402aee2e5c429884c2bbf5a4b075951ee49462cc2814b7dd28b7ee9025684512192fedc132f97db7fd912d6097cc4cc33240d83dc56eab0c149be761cec3d91ea1176db7769541e25e576332d0554cc8ffe43ab9563bdea5d33b14252c893ecfb0b5aaf2f74494a948add4023164f8b73621b33503e28cffa951eba90be00cec4b05f62da32b9f712708b9b2f9c90ae3132cbfaf9961eae6315449239eface42e40d2443e614302b5e54f835bc561081db3a9ba024c83943e04627e6c73dff4e634666f16da8382f90b7b5e7da59478950890f60b6fd18631460ac01ef9471d17110042648ea508836e7fec434773aa62224353ff74f281e5113f855a465764be75157816cfb0d1c7b6af83764c7f2ae1bb0d8b9f1b327e422cf16b4cdc880471bd18d562b7e86637a2978a18b981c6401607304669410e0960e3e706809a8e81789dda0f0a68166ea985ecb00b499c59308fdb3c70fb9270d221923ace7400cc6d77a7a0fa230726412ca548ba5d8a2fb0bd3ffddaf3b7368814688661c545c35340f361f246f996a5010bc460788f5c25f83189f809cad7a31d9a61dc1ab16f6d66b11097407622bad4cf2fff9137c8ee36f18e2290bda09fcddac1224f81a18ab73474c468360965287401bf96c9d81e0af6ca6b991324cbaf68c533eb428b5fcbd888df7e82d6376f3fbf8706cfcc0276229fed6fda0d2eac219a5e251383071fc0a7b207dbcafca858373de6e3ee7659e7bdccf7d526f2734753cf99359f99b52f1b73e897959e8bd4e3994da6d027419ca7ef3679193a128bb2ab41787b6619fe2dca8c161ba3bd0297610b85358a8b4ca9411a345008a601d4283c0aef5d1940f617b7ed67a307ce8ae3b2ae267c1384f412f06bc7d36155278f986f24ace71aec4c52fa9c8c308d8317f2644d71aa3732771833395665a04ded657c51c266779c415e7e38087b1d57d18fc07bd17415d94cc5db51597f0d94fdcb138a4e239891bd94a97f2a05a84df65396a79be2b2c9ec3fff12e3a31b80070a779f3f78f01104663ed50357762ce7e8668533c5461ec888dedadc55a78c362c755a5a90e0e560e5182f1ec76b388366ed9222706700aa5322fe24f6ab5b818590a03ebb9e6448e63f0507ea94fdc623f513cdef864455001843632432cb0e6c2e9369d187a720ec83db97a30b40383f52e5e146ab6eabd3c4704cead11e0d50cc6d9ebb6ac23ef29c9b754c934311ed6b009feb2bff4e46bdd02d27717f26bcb99632143e96193df08a8a1d01e5acdcf76d7b76648f7495ddc3ca882a0f2ba9ae0cd100639d49a3ecab7d52eb1d4ac1c7664c3a746ba8091e79852f93a83c93442660c103018ae4d733adf2e12785d0679d0b50e4ba26b9dd875d926fe09eca8686446c716ee2ddb8f9f7ecb53b41fc44446525b960684e832e6b5818146ba77c77902b6c1f176eeb9eb4285a04d13f95de51d946aed251d66b5324e4e792480461c04aec2d4a4f58358df9dc87724497114a05b7fb0acf4924edb17706c06ea04baee79eb50fdada7d56c21f454de4adb41d48631f903ff98558e133e8879bb29dd891a45be8ff44be93fa312e89b40d81217c20160ac8ec794e8f5ab1135cdb3698e58071258617f2a04bd6dc68b42ed1a42da51167061000f1582cd6b5aa2440d54b3e9050cfb2706754f91c077311badb8ee1257064cb90921ca35c85ae14b7237c56b06d0b918615909848ebafe660ba819120c2dd93ad96f839d43522d65b757517b1add1c58f49eeb3d6dcb00421d1462ee82e80cc7ba02a183349e11298e1301e573233759759783632d90edc1ff46aeeb5da961a12d769af684afafb7302d636b895c452e99b1ef8b5c7c6ab0ecc3de65ce7a5844a4a559b6047b6fe0e1bf07e5a9e4ed801da8a2efa5a4d29c1c15f64b7e384665ca784997e27ed13280e2a50b8ff9d464d2af7e9e876a9787baa88c24291c0e17c3b004c7f20ee2c41f9c7f2857724a25f295e37fb47dcc63d1040938c9a006a3bbbc002dc92c6f170017b1367dcd695ce03f052c8b8105527911a295750d3f526c629b6523b358b2353759d343272be74779725f366f1c4501b00959e3495a1630fb87ec9846e3af5f3e432f2d1e927193a7445c15483058ecf5719087a87c23618af94e3e8928e85483c8ca15886ee2fe84f9dd825f37c8f0bad778551776aa6cd0a7b232061f065522c0eb7a29fd30d6f2b284d453f9a3238f33ddd41be6d528addc2a5526973fef5f7fffb8f1bfcbc95cc7932c9e4d9eb03954d6dec10e09d56ecbcab2bd67336162613708782e04fe678d60d0518d10f47fd2b98f5420a3c6d39b414715bcc77a974a0993ca53c91781c54dcc2befff88c0f0ff8b35c3813fb12527b54b4d6c17e34ab79f9a04d667aa7fd5ecb2f64645ac001f76e4c71bb5e2930fd74ea29ebb73fdf14c6c18ac1aa2ea2ca2adb6e6eccfbe5041f23f08a9aa4e2ea62c14014334034b452ea1e750cbcc8b43e5701bc57be4cd4bbd39a959bd9d88d47573a95f8d969e0973c7c9af7631d34c0dfb1489060676fb8ed763509116f43bd912e435fbbfb5388274058c2a88f71489bafdf62fcc02762096321fac15372af1041b5e7e14bb900fb70b6eb02d4648a5a8eaf1d504fbb01d87ede8cda7c2d413437eb4686da4213c25d3fa94853d836fe696449cbba0946cea9d7d710a5c6348fe9b019bf710d1f530fa9645bc87a2f298258e741aa6def77ccc5070dfa3f6fcc9ec45761a3c5d911e43f49c30d31091df308fd369cf950cd6ec9418da6117aaddf4457e1583c0725f7dec3e7e08896de8696e493cfcc3f1fd95eaf7c4a55dec1ab36b3a6b8d84c43e436d97dd7e842845eecb6652b11126030eaf1c507d1c20")) // 3072 bytes
            .contentType(ContentType::BINARY));
    events.append(CloudEvent().name(String::format("abc_2"))
            .data(Buffer::fromHex("bd97c37bc5eceeb853922c891614cfe4b927880b3ca16ba767473ee7c1902f1d5e38dd41a648b30ba64221e0e047ea97896b6a19d98d103967900b0db9cda4fbd0baf6bf6b3dcc56c46dba364379824fb187721cc60160c2bb7344099f66c7a015db797c2b1f44b308425506d1ab9a17d0aef322751b65b718e3d1477a2c7e58b87798969c1b130e995c00d1d46f811a48dc3918d0c8cfe2e29d1c0ccd7ade45c6f191069fa53f2d6d308baa9b0eaab8924e3f489206c76c3ed38b43fc959a01af93bca2eccd814d1553935333a6e57b38b8f9b546044ccae0feaaa6b2893514ad3b1fe4c12f5ccebcfc93df41d3ab7baa52bad871fb8106f149ef3e9ea6ca2cb837efee49bd1cdde4732698f0f723cd540c572ff9fc03a204713078a84c75c9030aa0967494293594e8fcfc9fd9f561d10e82023de4ca2381d3d64a3551bfc718d309c307ae4915661231b63fd5fc7a11ff0511101db49e7ed1007df9e88131dcdf21994ad544003eb5c0f402cb210da22c251e9da15fbfd2b9b8779e00e9156b6ce04a7a55ce68206060cfe808703a9f1f458d559c818a96260814d64394cd0157871353192c8ddbfc641e53e898d5d4862c34b03558cc3259aadb0358db265b2cfd4045e45fd31b49b96ad46023ef1489842c3f47a9d8e976e79ba77c0d007c54c443cc62de4931dec70f9aabe0d678737854465ed95ea0b7058e7e079e074af1ba6a98b4f7e9c952e625474dd607d4dce92b435fa4532ffad951736dc1e70f130509bb3ec7c9a41cb9d4ff8fc14c522d9d6b8ba0af444717f0b1d52bc7fb06a27e4a81085d6a2935bda5187b9e19dfb00a9bb12efb089b51dc24aa72aa82ea979e478321557d05fc9b241e765202ec028d1c9dd03a6810965b02f3ca731112786b69cdb096a86cdc0c26ffa74a0f764fa3913ee35ab0b1b19eef0c2793f9830e9fb97fba160ac0f63cebf4ed3fff5d60fd21296d0a05c7bb5b9a6d98dedc77138b5cd72c6635f0e95bd780f89a78b555193650cf04b7da04acd881f00949208b54eae4790a97941ace1081f2e5adc0055b2cc67c50c414b35eddebcbc49eb71234bfd412ab78aaad7c8072bb2f840d0187df8410b498bab4245d9464dbf4fc5d4bac109eb33a82a66bbebc20835d6950634c4550ca127bcc8cb531d5d0e87d72383e37586cab3e6d2a957f308fcc7b77ebb3e3cc2e07db26c2f352a8c00c5f8b759f2fca12f50ec4cf093c22869b8b585a413c6e92fddd6046843199e2c46691a7675e045b405c0510845b784d5ffa3313056865b4ea7092411777175b442b0a4d46137134bd4aa887079cf477378eec1a6defe92e7996480d91592d0e797842a089a26be3b7ec8371556f0db43bba36cc8d0c62c24c274e508fa26108b8c1a8684cd7b217f2443a836c6824a10717bdb7564a6565d25ba5703cac4e11fe3872e2ee697e579c8631573d4f281ab8acf2575e02134d1155cc59af46f31f18556ddacc168473179e30de26b1f1ddde271ad18beae8b73bf24d86384c4cf8f48f2171b6e3dd4d31a70a5edb4d5e13a717cb7749c9c9739c6ab86393cfd6416006532a711c5c8aef1b119588526b8dc3b00f95433445f7d5ee3eb0e29e85e1a35d6561952abf67cafa368d1e117430d61fa05414ac272d44cb31adcc1476b80cee35f9b2dbd60898bb52449b9cd207bbd6ea7705dff061ee94e61a8d7163ebade59dbbcead52efb0835e3a91c1feb321904b219267b3d41f9047f11786cc89eb9d18adf69b208cfa24f65edf4ea9db4afe53f03dbfcd0ad6ed4986ce69e17c65650aa09c6c5f48268db8eabdf74991ba9666200dbf2743a51482d157de6c2d71520bfc90adf24b204f5a2d0edf9cbb35688f5ff8b1858030c17857e2e1358e6e3c911bb9736fdcfe82ae1143dfd6507fc03e58ffe3a3cc0f0ecca4a8d2e4abdf4dd56d5ce7a1fe084be48bb0ac87c59934dcdaf2d45a3cc23cb6115b1fae7499d8d7e5e333125721aa5dec28fd377cfaab7e00afa7304b3fffbeb5d7f3b21aa58b0ab6bab5dd84fb28a8d384f4ca934399fd0f4718edb21931bc1a1e8454acc7fc29ced03bc875744f8fa3dfeff091e48aca2d68eb1a7f68f17b758c147f08e3a2c6116638b748e5b36d8495907061a54b2c09d63833e4a883b2d16b92a4162563548a330aedc094f8e94bba4c4cf895c2b919817190d31d2d8e2d9d2bca31ff55eb6ea23a69df0e241352bb28370688e9ad9c10eeff5e5b807a4a543fb6d31c8c439792514c0729e259f1ee76e4da2aa4788190068bed9e49b027e311172cc0ece2bf19805ed65c16b921f0c9c9936fcb0e67e8a707e7b6fb4b9f8c7d3d68a4e6b49b171e45b48bf039355cfc1a7239881087d2432c84db799c0f5b8382f42af04ec3eaa9d022bdc7da831bd130ef68adf23d79d73cbf24eddfd4e060604daa00169ca81c9a2b552570569adc10594cd3b10619bf1a596ba2f538a31091e3ece6a9dfc2540e99d5520f516f16949f25827fbc3c2086837f03a78e9643e1704d6eb61a92101b2385a347d176de1eb32d39d85f180417b9aa4f5509881657b233935b55cfb95d0f3df3d7dc7681a879e274b5f9e5368188d6ebe94ec4e3feed046e87fe3fd3364aa49b782f1940c43845f1f96befc55599ca88d40dfe9887891baff3b1ee562c837ca647747e43a29ff37d836fa35321f4b06812ebd58a44a39fc3344b8183675ff49cf9b3b2927d58e3f3eef2f58c9d46e724aa99439c07d1ad156c9cf7883a68a720dd6705d6e41672a786eb6f87f3fc5b0fcbd1fb84949e4638d0559f323f43c01dce5a49c730d691101cfbea841c0f9edd57e49e2028a24669d82703dee503ace0390aac2e16273f9ed5aa77e4edf5839e22996feb3b564d230c9fd9e48bc2a18e29b387f1a80bcbd12db01343594cd2517ac714a72ceb6b4b27a9168e4a22c200c452f2aa27d065613ea6203b87e62b279124492b5d87364cdb1974866c8961f30b1a73f270feec9e1117245d8ecee4b8da3f2c73d13e4505c049a7009fdd912d639e93c7271b229f82d36274be71041d06a4e92829ca6404dfa338d2120bf6b51dcdbae4495260fcb7097aac97a95d8e7f2b4d7307545ccc704b3cc41d40fabfe9a103ff4dba848cc3223107e68d13ca6bd76b559e5218054a78ac956960811bfa4f5f0b267c8b9b6b7399351a41789bfd8b3877f68f4aea988ca6c96517d2f95946575a58c9ec88c6905ec4b1b41475c0ce325e6f8d1115334feb73a1cf755b462a2fc07448620abc2131af20b00a2fa86b5261eec6855d3dcdac82adf4082c6ba9eeb423b9cdc4c2c6cf6cdae43054d26e21d17310640e58773ca0a0c83765e81b0d54f05d6ae28fdf3e60b099325ba292b66eed7c6b103cacc1e0a4b366fc2a23ad2ca34434234217f3e739523266fe206867a08e52b68a36dda97283d229f307374f7cc7defef3a8a2d226515fc0508fbde12565f861f6c6972ba4b99ae599624adbc93ff9e6bcc7ccd2ae3d313f5aefdc42f1f9669dd76377d7be2d5cafd98bddab396791024b1788554594a9c7f41810b5d3f9f0d78ab176dc7b4a9ea397b67c01559babb4b2c259075a5a248590749d7fbf7afb95b981521cf86ee872176c0e2d77e25d6789c64102b02a873d6e838664e83cfac8f197af89fb0467114e85f08dcf8837999d0822bea97e37361279592dd256a36591dabf929c6c19d26795f6d7b02ae6c8c76a979275adce8d86994cf121404ef999947d0327f53765cbb52be9a72de5facf5c7679eff959706b88c0de594c132422428ccbbdc3dfe52a79d42aae584e27636be9af52194edb5213c13f9b95a71f5cfe25a0ac166fd57da177dff645a8abd22710246df84ad26cacefb37f88ad2e1202be17e05b0b30546c327a86f64b6c8ed2bbbe3d5744ddc8037b7de89f3d85520ac11ec23aaae7e88c014358482663385064fd844309b7d590c49fc3b0d1a698f4d387286aeae5b005ef9597fb6eaec93e2cd9adf0dc996cb8fc2d44466e9436d29f702fc4be416ea81c218cc70d3803c71c492a167d796dfcd68461560b8bda9e3dfe0fc5be68589fdbf8fbccdc13cb10405096b826dbc067fd45e04d7e23e2d98743d9f4da44ceb9a140541d1dc24536686fdbdf834418d4211fb84ca01bafcb17b485dda8057e086f18fa150f0f2ee0911111a77f377e9a7ff2a18370fa1cb12e749950de49f545775b7d845a1007cf5d24abd421f93a0d64b29b8d387041f2a5460dc2c97c5eb1f3a7a2ea2ca6c8b8e5fcb1c1a6b5ba16ee4288b1feb4faa564a5ea6fd2864a57ef73ebca8ed55f38706203b455714fdbcb37ca3b26")) // ditto
            .contentType(ContentType::BINARY));
    events.append(CloudEvent().name(String::format("abc_3"))
            .data(Buffer::fromHex("7223881577ba8e7fcbb695e76ca8b950a1ab001006f6cab5fffa554faa1b86e534bc00f34e784c909be807d2dcd9588922df1237463e7048ce7fabeb9b18ae1037f5f1838c90fb08a54674da9e2a615aa76736bcffb7d8a0080421abe1015a3ce55e9059827ad3e9f68e03af009711247f96a07fb29f7d9f11d601e9a49f89364184e9a44bdc9fed62061aa06437114f339e1a3b58f385a479f5e98d495636ddc9a4fbf45cb84f6fe385318f09a1adf768a74f08cce3886db139bd4ec2d19f405875c646082915048d6716b1cb73a477cfed327a902aef93d0134e92f801ab1332539e91348f3bd8da90dfae1d029f53ba4ccc9faae33688b2466c8bf308b41125adbdb8624ddf272aa7f1cfc34e6e84e726611ba9236cd4155cb9a4c4f58d9487a9d700db30a43bece5ec78080891be1024b1d2072721a6f82b210eb316af781754f11efe3137935e15aca077c80c91ecfc300130bcf4217ffbea2cfa53efdc43585ad319235cb46c637d8f33c05022dbbb3b54732f7caeb22afb938905f3ad77dff75c531d038149d98b227897c5f2bc331b3e4c93377294bdd3066a9d981139af9c37da37c624e57dd799f77392e8dbd96f60b05c0964ebfde1680e8b200ff1cd9a082f733b5a1cf1271e64cceb0e3b56d0a8cd051a7616ac0279f3c48fc03fb77d3e88191fca840a3657045fb2457bb2283747ba37559f111f9d8ecda4090ce31cf401292c49f5b02f406b47132cd8e973b1d6c82c13835dc8dacc2559b36311c66fcadf48fd9b45e5a4a25c0a6986915ac841d642d40c7b9cdbc0cdb7bb9b9d3070cdebba979ebc55d18ae56b26d02f5a99ad073ddade08cb8bc880d93c369f2c75769c165f8fc54e516fdab38208488c86bb008aaa9b8768b904c34142821e0af78a51afaeac6eff1c318e6b35c4ae4669f6f4430a9a70aab4bc489e984b9f3a2b69a32e72463e6fab92086279280252cc145bdaa76dd55decf66863c17a55d8a4284ac9779aede4cf62502a2f462b52ca4d071c0698da752978a6c145659c4b86dae758f33b32a8c400ba0469bf947b50e32aeeeb2ed79e73beffd393ea412184201bb8d3362c92cb020aabba35e3ac9bcc49d8dab52d5dbb2cc966ad716193d93235094e5a130967a016e6d89e2d064a73f3abfc332521b94ee24f9704c12b5cd33e3c26166d3e001a2147f7b3a79879f68496cc00f14f1465b9ddc00a0c58befb0dcb55cd90f57993612e625c8e635d2d6a8293bdfadee21e5c9638754cc288489cfde5ead9502ef81b5bbde703978cef0b1eddfe23f8f6cbe6c08eb3ccdbbe08fcbfec7d876bad76d64fe0e75197801a55299b24e61dba95869e7cd4244a01c0afabffe4b28a173bb5c8730af0bc289a8ecaf566bf4bb7c197ec781f8050e4487a030069d124530eec524af42a00d7ae04fe491408b21d72b1daeee7e4d5031bc4c2ebf235d1a4ca37262f91c8faa0dc0dc426018b8848cbcfa7cbfaf58dbda7bb81568a7d5e5dfc78daf04a0896210fd6ae0be9c12f6d0f95d26a62eb09419a4a188b4a47c70655ade0220efbf79818f57ae4ee1132f90ee4f497a030d97b637bd20c40a98bd77a99e9a682a0b44ecb68c1ee33f283b2fc3df08b4735bee93b123bb028129149fe59fff3e69dfad90a32af1ee453511f8ff77161d0f8481a8cd4a73aebae2a10ae99e49cd835fcdab2f06e5b36806265c3dba6a0f4eac7e2be9c14f6c0345fadd78684f379c43658677a90c4326e422e9283d665225fec0a45d3223ec6b445a07b8da8e11d789dbd5a9a949e508fac64ebff3477a2d14579631f65271957d7e8d3373bfaa7262cedfb4d978ddbfd8b4f1d3cecec9818c8a8b8fb3422e6589d0a231845859d2a5def89df7d409e02782bc4dad51abdd60c8d0bbcc31a4bb3c1ce7d84bba0e733667f2f2ff46410d31b3f20fef1114854a43e821d64d3dc35d8fc5f255f288035ea0d191b2d58adfd105e9ac0a3de9d9904f8c5787dc0a943dc603f2a7551c02cc39082eb7bca0d30fa60d66ba580887b58e9ed703afd349345f2931d80c79dead2172fecb584027199db4ea3659a5bad0dff21131a7167a4adf7fb51fdbfef97c8ecaf7cc072df32582db93e18fb3790addb0bbca55e5f8f365dcb2f0f398e1a3ac3ce4f552231ed0f172fc7d276f796f128de25c5da22aac62e38a87248fded3f00aae861e1b367e7c2c57832d1b4d21e4d6d9ae26082ab8905cc8c933285893c324169c5c7d19cc31bc489d9eade0c8416d5bca5477729a133439ca982547869eb84212804695b70819bf00008ce0beb6d8472948a7e344d86b128f15d7a4d0e4ea0a74c586c77bd592a825007567923c6f217862036dbaf48052a754c734adbb2a21104216f799161724631f38c4b9be0a5c07c5a2586cb8935a1d87e68492ac9573c81e7462acdc4f9e034e6dc55450f0e1d77036c82242b2271b4131770e283282602cdca1ce8778ffd29b3a63030fd3306eb5f802994c8e21384712d5f347878b227a4ac835e856becb2cd58c46a9475a4aeec0c3fe6dd423845e3b5881d6e9b0ac5193f17b05240b442924878d90e0b59ee1303d104736c6d1a23db0e711953b0e3fbb150fd27ca5741af7022b63e23d3f07bf324d767c429b72af2fb5f1049144a5e93f5029d01d4605b4667c0ea56516f8000e890c513b885b8c3cffe659ce46b8eaf64df126ba89a4ef5f805ea45271846ec7757af4a6539eefe9e9aff881de4f5c1a043a0b4d186ee74dddd6e2b853fd36713d800e02fa4cd91a95ff3d844fd16b47c47d5a741adcf9451eaa517c366a374a6a331de557c4561381e457a6cc97c35dcf7e51d121ea44e750462d8ffff2437bc4721b9077204269925e362f170b1a9ae0ffc7d2417a2801df15582855728124e6f76903083b2cec5ff446f9fa51b4d49d54a746b53d892221098acdbbed07426201fef538f81f5549f8c490e0d35abb293b1860ae790a3a3021e5062e84f4b7beea9f992022df286b47fd75bb4e9d6be029e0480fd8694269d2bd7b50e159e38cc02c52e591407c0521e94878565e51286f7037a38ee59fa4e3011510ccbefc3f705298258b638e33722cfe2383e25dd91426dda8a251adf5e88cb1e318a84f58a317c8cfaeb357ca1654675abbb0f4ff74df9438ad1bf939bcce973daf355f58c2ca19acff957becf86cadbc7ca4bdd59ede8966f60a49f984d8889af9bdd2123272b4d2ceed641d5490a9ca2d677d81c74b38d6e32172fe91e93bf5f6ccd10cd1ca15072ffa0442cd29f210a25f61d550cd2a1f9f8bedb88a53a4b5ad35ab4a7f1496178e11836b65e08aa88379f07bf6c3dd6b250e5ddd4ab3deb8a8d68c79a3e9bf034281f19afe0b4dfb06434de69c7cfa80653da4f7c371e5b851c0e8209fd5fdcfc8a8f67060ac4f434ec7836d6373a829643726191ce8ba240176d52be4f9ba3c97c40ca9a2aabe498da360ba8cf52ead65b3040c1c2bc14c203e3e02a8312470305df7c6329460510cde9e41f7dea35ae7bbe68a300b1ae2d260a4c24cbac5e0b9b4bbabd23b50d3b402a5bedd68bd7766d58da9b21da051edc9f69ac5ba0452ac651b3dfdfe99c9837dc67f00818b36f205f2f2f4c496ed5b6aea56bfaad62e645a1e45b8b199797541a702b79e0fba2d2353c0950af2be359c03d155ee2f8c77828d464d488d57afacf0f24af56e2bb49a3c5492e0910ac90fc74c0a9091856a209c472f258e728e8a5032c16d092e7d7378a3ac6e57195ff3df1d7a15365fc5419167c3b33031c7acec4a7eb6c5c97c0c6d88caf49fb82d8514564efb7cb1538fa086ae62b504a12f0ebd8950764a1c96f196eb0f3dcf7cef0c7bc0fd804097cc6faf566104fd5e1035105e09081aac9943db086b89a2d108735ceb7cfbe2e2cc276e25e53d90ef126c9b238dda394ca4cf2356abd53d6043a75cde74498cd5d191f42a40d8050ed62a7e9aff8cc6f28a75357354ebfeed9b6e25f9de973ec6dc740a9b9674a096ca487a6eb867ebbc3fad215c70558518cfd78c70f61fcd1407c91bba9f554830ceb8c825a72428d227324d543080b6cc94a1cdd15e17c58314f2dccf956ddcaca131c12c6e06dbfeba2966f6992581e9e09a2ba0915a73b6e27a57234639ac36a4330609994cf7ce064c3d2429fe0d1fd6006789d8515e2c1d377ad583958ec28ad0cb0296ce21ac4369816ee94c60762aeb0d7bd433242c43572e157aef73b6d3ab9c89b098f25896400ce61febf1bed56106c905f924dbd1ac00d6b5044b2a16a80b9d6713ca3cbb850be8437c765e3d006f7e36aea19c07591f251703c1be73c8b975bb4d652a1")) // ditto
            .contentType(ContentType::BINARY));
    for (auto& ev: events) {
        assertMore(ev.size(), COAP_BLOCK_SIZE); // Sanity check
        Particle.publish(ev);
    }
    unsigned timeout = 60000;
    bool allSent = false;
    auto t1 = millis();
    for (;;) {
        allSent = true;
        bool failed = false;
        for (auto& ev: events) {
            if (!ev.isSent()) {
                allSent = false;
                if (!ev.isSending()) {
                    failed = true;
                    break;
                }
            }
        }
        if (allSent || failed || millis() - t1 >= timeout) {
            break;
        }
        delay(100);
    }
    assertTrue(allSent);
}
