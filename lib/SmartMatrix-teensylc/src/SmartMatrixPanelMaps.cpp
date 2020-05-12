#include "SmartMatrix3.h"

// use this for all linear panels (e.g. panels that draw a single left-to-right line for each RGB channel)
const PanelMappingEntry defaultPanelMap[] =
{
    {0, 0, DEFAULT_PANEL_WIDTH_FOR_LINEAR_PANELS},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod2[] =
{
    {0, 71,  -8},
    {0, 87,  -8},
    {0, 103, -8},
    {0, 119, -8},
    {2, 72,   8},
    {2, 88,   8},
    {2, 104,  8},
    {2, 120,  8},
    {4,  7,  -8},
    {4, 23,  -8},
    {4, 39,  -8},
    {4, 55,  -8},
    {6,  8,   8},
    {6, 24,   8},
    {6, 40,   8},
    {6, 56,   8},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMapHub12_32x16Mod4[] =
{
    {0,  24,  8},
    {0,  56,  8},
    {0,  88,  8},
    {0,  120, 8},
    {4,  16,  8},
    {4,  48,  8},
    {4,  80,  8},
    {4,  112, 8}, 
    {8,  8,   8},
    {8,  40,  8},
    {8,  72,  8},
    {8,  104, 8},
    {12, 0,   8},
    {12, 32,  8},
    {12, 64,  8},
    {12, 96,  8},
    {0, 0, 0}   // last entry is all zeros
};


const PanelMappingEntry panelMap32x16Mod4[] =
{
    {0, 0,  8},
    {0, 16, 8},
    {0, 32, 8},
    {0, 48, 8},
    {4, 8,  8},
    {4, 24, 8},
    {4, 40, 8},
    {4, 56, 8}, 
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod4V2[] =
{
    {0, 15, -8},
    {0, 31, -8},
    {0, 47, -8},
    {0, 63, -8},
    {4, 0,   8},
    {4, 16,  8},
    {4, 32,  8},
    {4, 48,  8}, 
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod4V3[] =
{
    {0, 8, 8},
    {0, 24, 8},
    {0, 40, 8},
    {0, 56, 8},
    {4, 0, 8},
    {4, 16, 8},
    {4, 32, 8},
    {4, 48, 8}, 
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap20x10Mod1[] =
{
    {0, 14, 2},
    {0, 29, 3},
    {0, 45, 3},
    {0, 61, 3},
    {0, 77, 3},
    {0, 93, 3},
    {0, 109, 3},
    {1, 12, 2}, 
    {1, 26, 3}, 
    {1, 42, 3},
    {1, 58, 3},
    {1, 74, 3},
    {1, 90, 3},
    {1, 106, 3},
    {2, 10, 2},
    {2, 23, 3},
    {2, 39, 3},
    {2, 55, 3},
    {2, 71, 3},
    {2, 87, 3},
    {2, 103, 3},
    {3, 8, 2},
    {3, 20, 3},
    {3, 36, 3},
    {3, 52, 3},
    {3, 68, 3},
    {3, 84, 3},
    {3, 100, 3},
    {4, 6, 2},
    {4, 17, 3},
    {4, 33, 3},
    {4, 49, 3},
    {4, 65, 3},
    {4, 81, 3},
    {4, 97, 3},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap16x16Mod1[] =
{
    {0, 64, 4},
    {0, 80, 4},
    {0, 96, 4},
    {0, 112, 4},
    {1, 68, 4},
    {1, 84, 4},
    {1, 100, 4},
    {1, 116, 4},
    {2, 72, 4},
    {2, 88, 4},
    {2, 104, 4},
    {2, 120, 4},
    {3, 76, 4},
    {3, 92, 4},
    {3, 108, 4},
    {3, 124, 4},
    {8, 0, 4},
    {8, 16, 4},
    {8, 32, 4},
    {8, 48, 4},
    {9, 4, 4},
    {9, 20, 4},
    {9, 36, 4},
    {9, 52, 4},
    {10, 8, 4},
    {10, 24, 4},
    {10, 40, 4},
    {10, 56, 4},
    {11, 12, 4},
    {11, 28, 4},
    {11, 44, 4},
    {11, 60, 4},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry * getMultiRowRefreshPanelMap(unsigned char panelType) {
    switch(panelType) {
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD2SCAN:
            return panelMap32x16Mod2;
        case SMARTMATRIX_HUB12_16ROW_32COL_MOD4SCAN:
            return panelMapHub12_32x16Mod4;
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN:
            return panelMap32x16Mod4;
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN_V2:
            return panelMap32x16Mod4V2;
		case SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN_V3:
            return panelMap32x16Mod4V3;
        case SMARTMATRIX_HUB75_10ROW_20COL_MOD1SCAN:    
            return panelMap20x10Mod1;
        case SMARTMATRIX_HUB73_75_16ROW_16COL_MOD1SCAN: 
            return panelMap16x16Mod1;
        default:
            return defaultPanelMap;            
    }
}
