HAL API Test Plan
===================

`cellular_globabl_identity`
---------------------------

**Given:**

- The test application `<particle-iot/device-os>/user/tests/app/cellular_global_identity/cellular_global_identity.cpp`
- The Particle CLI installed on local machine.
- A cellular enabled Particle device.

**When:**

- The application is compiled, installed and launched
- You have subscribed to your log messages via `particle serial monitor`
- You have subscribed to your published messages via `particle subscribe mine`

**Then:**

- A log message similar to the following will appear every 30 seconds.

```none
0000012889 [app] INFO: /%%%%%%%%%%% Cellular Global Identity %%%%%%%%%%%/
0000012889 [app] INFO:  Mobile Country Code: 310
0000012890 [app] INFO:  Mobile Network Code: 410
0000012890 [app] INFO:  Location Area Code:  25876
0000012890 [app] INFO:  Cell Identification: 88078104
0000012891 [app] INFO: /%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%/
```

> _**NOTE:** The resulting values can be confirmed by entering them in the corresponding fields at https://opencellid.org._

- A JSON message with the following values will be published

  - `network.cellular.cell_global_identity.mobile_country_code`
  - `network.cellular.cell_global_identity.mobile_network_code`
  - `network.cellular.cell_global_identity.location_area_code`
  - `network.cellular.cell_global_identity.cell_id`
  - `network.cellular.radio_access_technology`

```json
"network": {
  "signal": {...},
  "cellular": {
    "cell_global_identity": {
      "mobile_country_code": 310,
      "mobile_network_code": 410,
      "location_area_code": 25878,
      "cell_id": 88213008
    },
    "radio_access_technology": "LTE"
  }
}
```

> _**WARNING:** A compilation error will occur for all "non-cellular" devices._
