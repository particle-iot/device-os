set WICED=r:\sdk\spark\core\WICED\WICED-SDK-3.1.1\WICED-SDK

pushd %WICED%

.\make demo.soft_ap-BCM9WCDUSI09-ThreadX-NetX-SDIO
.\make demo.soft_ap-BCM9WCDUSI14-ThreadX-NetX-SDIO
.\make demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO

popd

@REM update platforms (any RTOS)
call update.bat lib %WICED%\build\demo_soft_ap-BCM9WCDUSI09-ThreadX-NetX-SDIO\libraries
call update.bat lib %WICED%\build\demo_soft_ap-BCM9WCDUSI14-ThreadX-NetX-SDIO\libraries

call update.bat lib\ThreadX %WICED%\build\demo_soft_ap-BCM9WCDUSI09-ThreadX-NetX-SDIO\libraries
call update.bat lib\FreeRTOS %WICED%\build\demo_soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO\libraries

