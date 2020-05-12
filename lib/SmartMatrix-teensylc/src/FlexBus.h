#define FB_CSAR0		(*(volatile uint32_t *)0x4000C000)  // FlexBus Chip Select Address Register 0
#define FB_CSMR0        (*(volatile uint32_t *)0x4000C004)  // FlexBus Chip Select Mask Register 0
#define FB_CSCR0        (*(volatile uint32_t *)0x4000C008)  // FlexBus Chip Select Control Register 0
#define FB_CSAR1		(*(volatile uint32_t *)0x4000C00C)  // FlexBus Chip Select Address Register 1
#define FB_CSMR1        (*(volatile uint32_t *)0x4000C010)  // FlexBus Chip Select Mask Register 1
#define FB_CSCR1        (*(volatile uint32_t *)0x4000C014)  // FlexBus Chip Select Control Register 1
#define FB_CSAR2		(*(volatile uint32_t *)0x4000C018)  // FlexBus Chip Select Address Register 2
#define FB_CSMR2        (*(volatile uint32_t *)0x4000C01C)  // FlexBus Chip Select Mask Register 2
#define FB_CSCR2        (*(volatile uint32_t *)0x4000C020)  // FlexBus Chip Select Control Register 2
#define FB_CSAR3		(*(volatile uint32_t *)0x4000C024)  // FlexBus Chip Select Address Register 3
#define FB_CSMR3        (*(volatile uint32_t *)0x4000C028)  // FlexBus Chip Select Mask Register 3
#define FB_CSCR3        (*(volatile uint32_t *)0x4000C02C)  // FlexBus Chip Select Control Register 3
#define FB_CSAR4		(*(volatile uint32_t *)0x4000C030)  // FlexBus Chip Select Address Register 4
#define FB_CSMR4        (*(volatile uint32_t *)0x4000C034)  // FlexBus Chip Select Mask Register 4
#define FB_CSCR4        (*(volatile uint32_t *)0x4000C038)  // FlexBus Chip Select Control Register 4
#define FB_CSAR5		(*(volatile uint32_t *)0x4000C03C)  // FlexBus Chip Select Address Register 5
#define FB_CSMR5        (*(volatile uint32_t *)0x4000C040)  // FlexBus Chip Select Mask Register 5
#define FB_CSCR5        (*(volatile uint32_t *)0x4000C044)  // FlexBus Chip Select Control Register 5
#define FB_CSPMCR       (*(volatile uint32_t *)0x4000C060)  // FlexBus Chip Select port Multiplexing Control Register

#define FB_CSAR_BA      ((uint32_t)(((n) & 0xFFFF) << 16))  // Base Address

#define FB_CSMR_BAM     ((uint32_t)(((n) & 0xFFFF) << 16))  // Base Address Mask       
#define FB_CSMR_WP      (1 << 8)                            // Write Protect
#define FB_CSMR_V       1                                   // Valid

#define FB_CSCR_SWS(n)  ((uint32_t)(((n) & 0x3F) << 26))    // Secondary Wait States
#define FB_CSCR_SWSEN   (1<<23)                             // Secondary Wait State Enable
#define FB_CSCR_EXTS    (1<<22)                             // Extended Transfer Start/Extended Address Latch Enable
#define FB_CSCR_ASET(n)	((uint32_t)(((n) & 0x03) << 20))    // Address Setup
#define FB_CSCR_RDAH(n)	((uint32_t)(((n) & 0x03) << 18))    // Read Address Hold or Deselect
#define FB_CSCR_WRAH(n)	((uint32_t)(((n) & 0x03) << 16))    // Write Address Hold or Deselect
#define FB_CSCR_WS(n)   ((uint32_t)(((n) & 0x3F) << 10))    // Wait States
#define FB_CSCR_BLS     (1<<9)                              // Byte-Lane Shift
#define FB_CSCR_AA      (1<<8)                              // Auto-Acknowledge Enable
#define FB_CSCR_PS(n)	((uint32_t)(((n) & 0x03) << 6))     // Port Size 0: 32 1: 8, 2: 16
#define FB_CSCR_BEM     (1<<5)                              // Byte-Enable Mode
#define FB_CSCR_BSTR    (1<<4)                              // Burst-Read Enable
#define FB_CSCR_BSTW    (1<<3)                              // Burst-Write Enable