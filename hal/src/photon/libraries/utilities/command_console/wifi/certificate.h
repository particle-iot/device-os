/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_USER_CERTIFICATE_STRING  (const uint8_t*)\
"-----BEGIN CERTIFICATE-----\r\n"\
"MIIFFTCCA/2gAwIBAgIBBTANBgkqhkiG9w0BAQUFADAjMSEwHwYDVQQDExhXaUZp\r\n"\
"LUludGVybWVkaWF0ZS1DQS1zdGEwHhcNMDUwMTAxMDAwMDAwWhcNMjUwMTAxMDAw\r\n"\
"MDAwWjBVMRUwEwYKCZImiZPyLGQBGRYFbG9jYWwxGDAWBgoJkiaJk/IsZAEZFgh3\r\n"\
"aWZpbGFiczEOMAwGA1UEAxMFVXNlcnMxEjAQBgNVBAMTCXdpZmktdXNlcjCCASIw\r\n"\
"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMz28dku/sZQtI8eRL+Th9+Ssl7f\r\n"\
"Me9LQJ0gxIh7TZZgOCbKRaQuz5kCiTcuq11zbBfIiiriwZ2znr6K2/0END4IRBsP\r\n"\
"/ragD+FR2Ggnavy+i5X12QU+R90B04QXB+sNpj3G8tfkfcuf6PE7A3L8cDibfwCz\r\n"\
"r9hPJnCsTcZykZrfHUPVIuF+cEd08lEeIJW1XqzdEHVy2xYbEORA8OOGzPA1uTaG\r\n"\
"Wi/QsxLaj1BhtMdAv+yBg6p6adOMna8N1QpwtTW0vtbv6iUv25k0pAQJUPVL6v0Y\r\n"\
"VRYtniPi97rXEpIwwdvlDlNYxwwMVfRkQq6aek8PIsNE96/3aXbP/OC76TMCAwEA\r\n"\
"AaOCAiAwggIcMD8GA1UdIwQ4MDaAFLCM4rLBhvxWYXi/HwbV8xGSsTgAoRukGTAX\r\n"\
"MRUwEwYDVQQDEwxXaUZpLVJvb3QtQ0GCAQMwQgYJYIZIAYb4QgEEBDUWM2h0dHA6\r\n"\
"Ly9zZXJ2ZXIud2lmaWxhYnMubG9jYWwvY2EvY3JsL3dpZmlpbWNhc3RhLmNybDBE\r\n"\
"BgNVHR8EPTA7MDmgN6A1hjNodHRwOi8vc2VydmVyLndpZmlsYWJzLmxvY2FsL2Nh\r\n"\
"L2NybC93aWZpaW1jYXN0YS5jcmwwTgYIKwYBBQUHAQEEQjBAMD4GCCsGAQUFBzAC\r\n"\
"hjJodHRwOi8vc2VydmVyLndpZmlsYWJzLmxvY2FsL2NhL2NhL3dpZmlpbWNhc3Rh\r\n"\
"LmNydDAgBgNVHRIEGTAXghVzZXJ2ZXIud2lmaWxhYnMubG9jYWwwMwYDVR0RBCww\r\n"\
"KqAoBgorBgEEAYI3FAIDoBoMGHdpZmktdXNlckB3aWZpbGFicy5sb2NhbDALBgNV\r\n"\
"HQ8EBAMCBaAwHQYDVR0OBBYEFAvBhZsRvOlqwE4Is4e8Ka81blgjMB0GA1UdJQQW\r\n"\
"MBQGCCsGAQUFBwMCBggrBgEFBQcDBDBEBgkqhkiG9w0BCQ8ENzA1MA4GCCqGSIb3\r\n"\
"DQMCAgIAgDAOBggqhkiG9w0DBAICAIAwBwYFKw4DAgcwCgYIKoZIhvcNAwcwFwYJ\r\n"\
"KwYBBAGCNxQCBAoeCABVAHMAZQByMA0GCSqGSIb3DQEBBQUAA4IBAQAPGCPlaHYA\r\n"\
"efDmwaFHJuB7+VpqivrSkFI9mIZaQ0vnbAdzQ56MbKelkijoS7nphpq52pz0VDgT\r\n"\
"yKGvWs3LkmLvRcVXWObCjvzChMf4ufCQRaQEKWHEedjL0+jwGZpyOMqvcfj3ll7N\r\n"\
"TWQ1l2filgHwNl7WXcx7AzzeZ61EdYbuHCER/vBEvInEDCAnNlQDk8FXqRyqa4Df\r\n"\
"Q8rHP32r1lKPwgTcCEXVn+y9cBim+qbtW2Zw/LAx6T3bYsze9pHWEMw9fSxGCPgf\r\n"\
"VjUmqpJUXtygDwa4/OY48KpglvetwKGGg4zNbDC5ucG7KY/kD3XG/YHffu4lenab\r\n"\
"gELU9DvJIT1+\r\n"\
"-----END CERTIFICATE-----\r\n"\
"-----BEGIN CERTIFICATE-----\r\n"\
"MIIEVzCCAz+gAwIBAgIBAzANBgkqhkiG9w0BAQUFADAXMRUwEwYDVQQDEwxXaUZp\r\n"\
"LVJvb3QtQ0EwHhcNMDUwMTAxMDAwMDAwWhcNMjUwMTAxMDAwMDAwWjAjMSEwHwYD\r\n"\
"VQQDExhXaUZpLUludGVybWVkaWF0ZS1DQS1zdGEwggEiMA0GCSqGSIb3DQEBAQUA\r\n"\
"A4IBDwAwggEKAoIBAQCi/p4AsGV9dkTmCjPQ8vLA7vdAIUUAvMpPUQ2kqh7xN82c\r\n"\
"4/4xvtVkSDHsTTgZMWNc+wxGt0L+5XgI5evJx6ZSDWKiBtERTudJW8KnuS9D9uD7\r\n"\
"vk3pMRhVqHMMJdXtZk3D7WtxdO+0UZAyWoO5bkifDtQhyIlgULmsKAbjqjapFpJO\r\n"\
"0yoGrj2LGpIAmUk+lSlWn1tFk5Ct2j5BDwPlgSJTUkt/Qpi7Yh7p9l3kz2ldXBCf\r\n"\
"J3O47tr2zENW0yIpGn7lirdBz8iUADE0iSprJzWbFr7/VBLqGMKXOujhZ70CCTRM\r\n"\
"WLh53hrbE1xQYZGTZSZNHxFjGyhrRZq7BuFMIUk3AgMBAAGjggGgMIIBnDA/BgNV\r\n"\
"HSMEODA2gBTXHXTI1mqUjBDPBU/jlqbWz7Ji86EbpBkwFzEVMBMGA1UEAxMMV2lG\r\n"\
"aS1Sb290LUNBggEBMEEGCWCGSAGG+EIBBAQ0FjJodHRwOi8vc2VydmVyLndpZmls\r\n"\
"YWJzLmxvY2FsL2NhL2NybC93aWZpcm9vdGNhLmNybDBDBgNVHR8EPDA6MDigNqA0\r\n"\
"hjJodHRwOi8vc2VydmVyLndpZmlsYWJzLmxvY2FsL2NhL2NybC93aWZpcm9vdGNh\r\n"\
"LmNybDBNBggrBgEFBQcBAQRBMD8wPQYIKwYBBQUHMAKGMWh0dHA6Ly9zZXJ2ZXIu\r\n"\
"d2lmaWxhYnMubG9jYWwvY2EvY2Evd2lmaXJvb3RjYS5jcnQwIAYDVR0SBBkwF4IV\r\n"\
"c2VydmVyLndpZmlsYWJzLmxvY2FsMCAGA1UdEQQZMBeCFXNlcnZlci53aWZpbGFi\r\n"\
"cy5sb2NhbDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4E\r\n"\
"FgQUsIzissGG/FZheL8fBtXzEZKxOAAwDQYJKoZIhvcNAQEFBQADggEBAJTw1owK\r\n"\
"3hDQfaWw3v2NHsuK7GBmdCIxsXitDgJTmcZ1EGVgWVdgduWHWIK0b7YRuW2dCX9V\r\n"\
"ICikaUURSp5tONRrJroF2kwS40CG3X9Lx/G39K7hxbk7S4pkkVca8Z5VVhbOP9HY\r\n"\
"KUrCgZruksg8Q4WqLVsvnGgA11b0/oVf8kWauScZeFh5PNtTa0WU8iIJ4PXD/7w9\r\n"\
"7UEzY+U7TOJR0h36/NGilu5XTwjcMesfnb2fTHXAzU95H3xbamFc51Tzv7Tn6eQB\r\n"\
"7k4aD5rAInKwN5xR5i9RZOzfgG/ZCFtYrN0y9KYvTRgdkl4uFq929k0v5xP6dqci\r\n"\
"V0zlAVNpC/k0urU=\r\n"\
"-----END CERTIFICATE-----\r\n"\
"\0"\
"\0"
#define WIFI_USER_PRIVATE_KEY_STRING  \
"-----BEGIN RSA PRIVATE KEY-----\r\n"\
"MIIEowIBAAKCAQEAzPbx2S7+xlC0jx5Ev5OH35KyXt8x70tAnSDEiHtNlmA4JspF\r\n"\
"pC7PmQKJNy6rXXNsF8iKKuLBnbOevorb/QQ0PghEGw/+tqAP4VHYaCdq/L6LlfXZ\r\n"\
"BT5H3QHThBcH6w2mPcby1+R9y5/o8TsDcvxwOJt/ALOv2E8mcKxNxnKRmt8dQ9Ui\r\n"\
"4X5wR3TyUR4glbVerN0QdXLbFhsQ5EDw44bM8DW5NoZaL9CzEtqPUGG0x0C/7IGD\r\n"\
"qnpp04ydrw3VCnC1NbS+1u/qJS/bmTSkBAlQ9Uvq/RhVFi2eI+L3utcSkjDB2+UO\r\n"\
"U1jHDAxV9GRCrpp6Tw8iw0T3r/dpds/84LvpMwIDAQABAoIBABqVXMDCa6DlDsYR\r\n"\
"MvF1/QVuTVworp1OsU6v1U2uUF3UlPPTAD1PJPW/cnSJxQgV1EsxY1o0ltARX+Fu\r\n"\
"uSGTzgLNp6eq52pgUC71IpA/rIyjWg9VF5Sqgz+S4RAcvJYr6wfQKeb+Z6JlVx0S\r\n"\
"flwHibaN1GcO5xuzCFVMw7mdWm0vjz8vyvxEVez4lSKRhzx/rbmvbFT1TxrMHCZ0\r\n"\
"ExCFKbsFMqk8CWh5bjPjoyopOVvVAF296idGXQ3nX5/lualZOjMVOpmuGRppDd+j\r\n"\
"rZMUCk1+7rNi+pQfOCc/gUcRCLr3t7gVPACZ36rA4WOc4t24HFebXVooT4fgvFEy\r\n"\
"NXFUw0ECgYEA+p+K95Sa6x/rkgg73KV4OxNW5txnyDT9z6rX6U6/9gkWw4gWFHv4\r\n"\
"9CEoUm+j7+DHV2GWNIVlBVCSpBeiPXQMacUztfWrSoG9mQvpdJ5vwXIjDXHly+RR\r\n"\
"+RbjMf1koSa0xF+M8hcOX8nzRMDKtygPACbBXUwIZvKboW+/69mKxqkCgYEA0Vyj\r\n"\
"fi6ry/OoopGwNzpcZPliUm0FppK2KeVvfUNU7U4I+8CMYf9prPRKiob6nwpsgxWD\r\n"\
"GvH9DDLZw2bUeZ5BfgccB5ZvTovjvBX7J/cUZiUI70208kWudcCCtUdQ9VXJQFpX\r\n"\
"vw50j1sjqhBala/lpca7QA8Zk/yY+XuaKrPChnsCgYBR2Cwvse3tft0VMW8rHTZo\r\n"\
"RGaKucuCjBue57He1QLHPCyc6iIbymiAgRuD4EVvHr66gHnm6PEWjTt2Lumim/U5\r\n"\
"zVaXw4SOrlPWWReCKANi7v0XdOyQax2B9MF5H8DvB47c2j9TB8h/65lwCG2q5oAP\r\n"\
"kphu+Vd9FxlP3QiV4tL5EQKBgQCFmyfe26vY8PraHD0nUYArFBcR4O8tOQ01OWzn\r\n"\
"tHNbKWSEPIGZ/GQU8qUrOC5yFjXfhXfwVyOUiFL95v6LSlojMihKE0+fAZjoq+Jm\r\n"\
"w7/p25KTHLTvs1Y9YQhI5WUd24weHElI7NGntpLQ8bTNN75HB8bxf3FkRlvdQmrE\r\n"\
"+4iVRwKBgEAr82q1Crr1yps3Xxc5pMuD70OdsLOtzgspxFe9fXzHpPAgZRqS3kIG\r\n"\
"iOL+yeBXDutByp9QorQn5afJo4BJjnf3cEUrMucb0oxEr+ZKxiYu/R6CYiTDp3Fx\r\n"\
"BjKG6fhBonoiURm0KIqLXiW/k2iNXYsUwuOwBHCdlyRpcWnu7ZtU\r\n"\
"-----END RSA PRIVATE KEY-----\r\n"\
"\0"\
"\0"

#define WIFI_ROOT_CERTIFICATE_STRING  \
"-----BEGIN CERTIFICATE-----\r\n"\
"MIIECjCCAvKgAwIBAgIBATANBgkqhkiG9w0BAQUFADAXMRUwEwYDVQQDEwxXaUZp\r\n"\
"LVJvb3QtQ0EwHhcNMDUwMTAxMDAwMDAwWhcNMjUwMTAxMDAwMDAwWjAXMRUwEwYD\r\n"\
"VQQDEwxXaUZpLVJvb3QtQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\r\n"\
"AQC555sEVFOZC7ZtUJfIQjaC4JOkUbd+nmrcIj052lzQoilRj+E2o0UHzJwkydNb\r\n"\
"vDSD6/3iedZwERae2oPA/nCYL9sOnQCpfJm6u8TbSuLuRqylNRvCvfDgancpNG5t\r\n"\
"X1HeZ/0n8VACv+kblS6ZC7NNfLxeSyldUuMU3tNt+rQHbLeD51qzUcZzppbrly+6\r\n"\
"BPDy6+7AuJUpmMqvi8knrSKTF5yIUXmQMcufmEe2XAPRD5i/6jKXZdNU1XELUn9V\r\n"\
"p4ixCwTVhHGnYihTuzO+Ef2YfToDmBdzt9beAldHOZ0MJheqM0JEWrANjxr3vxi/\r\n"\
"LTIqMgxMusnqqS4HvkzSxFn3AgMBAAGjggFfMIIBWzAPBgNVHRMBAf8EBTADAQH/\r\n"\
"MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQU1x10yNZqlIwQzwVP45am1s+yYvMw\r\n"\
"QQYJYIZIAYb4QgEEBDQWMmh0dHA6Ly9zZXJ2ZXIud2lmaWxhYnMubG9jYWwvY2Ev\r\n"\
"Y3JsL3dpZmlyb290Y2EuY3JsMEMGA1UdHwQ8MDowOKA2oDSGMmh0dHA6Ly9zZXJ2\r\n"\
"ZXIud2lmaWxhYnMubG9jYWwvY2EvY3JsL3dpZmlyb290Y2EuY3JsME0GCCsGAQUF\r\n"\
"BwEBBEEwPzA9BggrBgEFBQcwAoYxaHR0cDovL3NlcnZlci53aWZpbGFicy5sb2Nh\r\n"\
"bC9jYS9jYS93aWZpcm9vdGNhLmNydDAgBgNVHRIEGTAXghVzZXJ2ZXIud2lmaWxh\r\n"\
"YnMubG9jYWwwIAYDVR0RBBkwF4IVc2VydmVyLndpZmlsYWJzLmxvY2FsMA0GCSqG\r\n"\
"SIb3DQEBBQUAA4IBAQCRdTUh+WTd+LFz32QZUPbJ3Qb++STu7ZsdT9p2B/zbhyAT\r\n"\
"tscIJZoIczLWttbkxwCI0mB9hU6D5p7RdWyodpYbkheZFYVvJ2yuSKEx0jTakOq4\r\n"\
"sDCFy6LCwI67RTE12ngU0Uo2TZe/MxrF5tqKnhXCh9GEMeu9uAT2l/o4plI0sx7N\r\n"\
"G/vCPkwvygpgl3TQkQygmOgXYyc7cqAHsC9vOgCv10zbN5y1Hme1gV7JucF74Ma1\r\n"\
"AozWbdQAC1d+95lQ1IO8j/TrrSD0InivVSzS3FXa7vLneyR2GbguHdo3GLuFd+/r\r\n"\
"LrQ+weBJZ7Sk+f+f3HaskrSW7Lo2RoDMpPmJPvl9\r\n"\
"-----END CERTIFICATE-----\r\n"\
"\0"\
"\0"

#ifdef __cplusplus
} /*extern "C" */
#endif
