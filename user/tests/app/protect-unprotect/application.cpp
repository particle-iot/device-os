#include "Particle.h"
#include "scope_guard.h"
#include "resolvapi.h"
#include "ifapi.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

#define CMD_SERIAL Serial1

Serial1LogHandler logHandler(LOG_LEVEL_ALL,
{
    { "app", LOG_LEVEL_ALL },
    { "system", LOG_LEVEL_TRACE },
    { "network", LOG_LEVEL_TRACE }
}
);

#define LOGICAL_EFUSE_SIZE 1024
static uint8_t logicalEfuseBuffer[LOGICAL_EFUSE_SIZE];

extern "C" {
#include "rtl8721d.h"

typedef uint32_t(*efuse_lmap_read_func)(uint8_t *pbuf);
typedef uint32_t(*efuse_lmap_write_func)(uint32_t addr, uint32_t cnts, uint8_t *data);

typedef uint32_t(*efuse_pmap_read_func)(u32 CtrlSetting, u32 Addr, u8 *Data, u8 L25OutVoltage);
typedef uint32_t(*efuse_pmap_write_func)(u32 CtrlSetting, u32 Addr, u8 Data, u8 L25OutVoltage);
}

uint32_t EFUSE_LMAP_READ_SHIM(uint8_t * buffer) {
    efuse_lmap_read_func efuse_logical_read = (efuse_lmap_read_func)0x1010ae99;
    return efuse_logical_read(buffer);
}

uint32_t EFUSE_LMAP_WRITE_SHIM(uint32_t addr, uint32_t cnts, uint8_t * data) {
    if (is_power_supply18()) {
        return EFUSE_FAILURE;
    }

    efuse_lmap_write_func efuse_lmap_write = (efuse_lmap_write_func)0x1010afad;
    return efuse_lmap_write(addr, cnts, data);
}

uint32_t EFUSE_PMAP_READ_SHIM(u32 CtrlSetting, u32 Addr, u8 *Data, u8 L25OutVoltage) {
    efuse_pmap_read_func efuse_physical_read = (efuse_pmap_read_func)0x1010aa31;
    return efuse_physical_read(CtrlSetting, Addr, Data, L25OutVoltage);
}

uint32_t EFUSE_PMAP_WRITE_SHIM(u32 CtrlSetting, u32 Addr, u8 Data, u8 L25OutVoltage) {
    if (is_power_supply18()) {
        return EFUSE_FAILURE;
    }

    efuse_pmap_write_func efuse_physical_write = (efuse_pmap_write_func)0x1010ab1d;
    return efuse_physical_write(CtrlSetting, Addr, Data, L25OutVoltage);
}

void setup() {
    CMD_SERIAL.begin(115200);
}

// useDefaultPins = true: primary swd pins false: alternate pins
// permanent = write to logical efuse
// runtime = write to REG_SYS_EFUSE_SYSCFG3 register
void configureSwd(bool useDefaultPins, bool permanent, bool runtime) {

    EFUSE_LMAP_READ_SHIM(logicalEfuseBuffer);

    uint32_t syscfg3 = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3);

    uint8_t swdDefaultGpioEfuse = logicalEfuseBuffer[0x0E];
    uint8_t sdioPadEnEfuse = logicalEfuseBuffer[0x0F];

    uint32_t pmuxEn = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN);

    Log.info("pmuxEn 0x%04lx syscfg3: 0x%04lx efuse E: 0x%02x F: 0x%02x ", pmuxEn, syscfg3, swdDefaultGpioEfuse, sdioPadEnEfuse);

    if (useDefaultPins) { 
        pmuxEn |= (BIT_LSYS_SWD_PMUX_EN);

        syscfg3 &= ~(BIT_SYS_SWD_GP_SEL | 1<<28); // BIT_SDIO_PMUX_FEN

        swdDefaultGpioEfuse &= ~(1); // SWD_DEFAULT_GPIO
        sdioPadEnEfuse &= ~(1<<4);   // SDIO_PAD_EN
    } else {
        pmuxEn &= ~(BIT_LSYS_SWD_PMUX_EN);

        syscfg3 |= (BIT_SYS_SWD_GP_SEL | 1<<28); // BIT_SDIO_PMUX_FEN

        swdDefaultGpioEfuse |= (1); // SWD_DEFAULT_GPIO
        sdioPadEnEfuse |= (1<<4); // SDIO_PAD_EN
    }

    if (runtime) {
        Log.info("Configure syscfg3 at runtime: 0x%04lx pmuxEn 0x%04lx", syscfg3, pmuxEn);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3, syscfg3);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN, pmuxEn);
    }
    
    if (permanent) {
        Log.info("Configure logical efuse 0x0E: 0x%02x 0x0F: 0x%02x", swdDefaultGpioEfuse, sdioPadEnEfuse);
        EFUSE_LMAP_WRITE_SHIM(0x0E, 1, &swdDefaultGpioEfuse);
        EFUSE_LMAP_WRITE_SHIM(0x0F, 1, &sdioPadEnEfuse);
    }
}

void logGpioRegisters() {
    uint32_t swdAlternate = ((RTL_PORT_B << 5) | 19) & 0x0000003F; // PB19
    //uint32_t swdPrimary = ((RTL_PORT_A << 5) | 27) & 0x0000003F; // PA27

    uint32_t pb19PadCtr = PINMUX->PADCTR[swdAlternate];
    uint32_t portBDDR = GPIOB_BASE->PORT[0].DDR;
    uint32_t portBDAT = GPIOB_BASE->PORT[0].DR;

    Log.info("PB19 PADCTR 0x%04lx PortB DDR: 0x%08lx DAT: 0x%08lx", pb19PadCtr, portBDDR, portBDAT);
    Log.info("PB19 shutdown %d pulldown %d output %d value %d\r\n", 
        (bool)(pb19PadCtr & (1<<15)),
        (bool)(pb19PadCtr & (1<<9)),
        (bool)(portBDDR & (1<<19)),
        (bool)(portBDAT & (1<<19)));
}

void loop() {
    if (!CMD_SERIAL.available()) {
        return;
    }

    while (CMD_SERIAL.available()) {
        char c = CMD_SERIAL.read();

        LOG_PRINTF_C(INFO, "app", "\r\nSerial Command: %c\r\n", c);

        if (c == 'b') {
            Log.info("Enter DFU");
            delay(1000);
            System.dfu();
        }
        else if (c == 'c') {
            // Print all relevant registers and logical efuse settings
            Log.info("Logical efuse: ");
            EFUSE_LMAP_READ_SHIM(logicalEfuseBuffer);
            Log.dump(logicalEfuseBuffer, 0x1f);
            Log.info("\r\n");

            uint32_t syscfg3 = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3);
            uint32_t pmuxEn = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN);

            Log.info("syscfg3 register 0x%04lx pmuxEn 0x%04lx", syscfg3, pmuxEn);
        }

        else if (c == 'd') {
            Log.info("Unprotect (permanent)");
            configureSwd(true, true, true);
        }
        else if (c == 'e') {
            Log.info("Protect (runtime only)");
            configureSwd(false, false, true);
        }
        else if (c == 'f') {
            Log.info("Protect (permanent)");
            configureSwd(false, true, true);
        }
        else if (c == 'g') {
            Log.info("Unprotect (runtime only)");
            configureSwd(true, false, true);
        }
        else if (c == 'h') {
            // GPIO HAL has logic to configure SDIO pin if SWCLK is set as SWD?
            //pinMode(SWD_CLK, PIN_MODE_SWD); // PB3 should configure PA27 as input

            // Writing to SYSCFG5 register, the DBG_PORT_EN and DBG_PORT_EN2 bits
            // Not clear what these do
            uint32_t syscfg5 = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG5);
            Log.info("syscfg5 before 0x%08lx", syscfg5);

            uint32_t newSyscfg5 = syscfg5 | (3 << 30);
            Log.info("new syscfg5 0x%08lx", newSyscfg5);
            HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG5, newSyscfg5);

            syscfg5 = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG5);
            Log.info("syscfg5 after 0x%08lx", syscfg5);

        }
        else if (c == 'i') {
            // Log GPIO / PADCTR registers as we try different configurations
            // to attempt to disable the internal pullup enabled on the alternate SWDIO pin PB19
            // A similar problem exists with the primary SWDIO pin (PA27) internal pullup

            // PB19
            uint32_t swdioAlternate = ((RTL_PORT_B << 5) | 19) & 0x0000003F; 
            
            Log.info("Starting State");
            logGpioRegisters();

            Log.info("SWD OFF");
            Pinmux_Swdoff();
            logGpioRegisters();

            Log.info("PAD ENABLED");
            PAD_CMD(swdioAlternate, ENABLE);
            logGpioRegisters();

            Log.info("GPIO FUNCTION");
            Pinmux_Config(swdioAlternate, PINMUX_FUNCTION_GPIO);
            logGpioRegisters();

            Log.info("SET TO INPUT");
            GPIOB_BASE->PORT[0].DDR &= (~(1 << 19));
            logGpioRegisters();

            Log.info("ENABLE PULLDOWN");
            PAD_PullCtrl(swdioAlternate, GPIO_PuPd_DOWN);
            logGpioRegisters();

            Log.info("GPIO OUTPUT");
            // This is the only way to drive pin low
            GPIOB_BASE->PORT[0].DDR |= ((1 << 19));
            // GPIO_InitTypeDef  GPIO_InitStruct = {};
            // GPIO_InitStruct.GPIO_Pin = swdioAlternate;
            // GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
            // GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
            //GPIO_Init(&GPIO_InitStruct); 
            logGpioRegisters();


            // SystemSleepConfiguration config;
            // config.mode(SystemSleepMode::STOP)
            //       .duration(30s);
            // System.sleep(config);

        
        }
    }
}