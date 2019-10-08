#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  									480
#define FRAME_HEIGHT 									854

#define REGFLAG_DELAY             							0XFD
#define REGFLAG_END_OF_TABLE      							0xFE 


#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);       
 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = 
{
{0xFF,5,{0x77,0x01,0x00,0x00,0x10}},
	{0xC0,2,{0xE9,0x03}},
	{0xC1,2,{0x0D,0x02}},
	{0xC2,2,{0x41,0x06}},//40
	{0xCC,1,{0x10}},
	{0xB0,16,{0x00,0x07,0x93,0x13,0x19,0x0B,0x0B,0x09,0x08,0x1F,0x08,0x15,0x11,0x0F,0x18,0x17}},
	{0xB1,16,{0x00,0x07,0x92,0x12,0x15,0x09,0x08,0x09,0x09,0x1F,0x07,0x15,0x11,0x15,0x18,0x17}},
	{0xFF,5,{0x77,0x01,0x00,0x00,0x11}},
	{0xB0,1,{0x4D}},
	{0xB1,1,{0x47}},  //5D 47
	{0xB2,1,{0x0a}},  //07 82
	{0xB3,1,{0x80}},
	{0xB5,1,{0x4a}},  //47 42
	{0xB7,1,{0x85}},
	{0xB8,1,{0x20}},
	{0xB9,1,{0x10}},
	{0xC1,1,{0x78}},
	{0xC2,1,{0x78}},
	{0xD0,1,{0x88}},
	{0xE0,3,{0x00,0x00,0x02}},
	{0xE1,11,{0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x40,0x40}},
	{0xE2,13,{0x33,0x33,0x34,0x34,0x62,0x00,0x63,0x00,0x61,0x00,0x64,0x00,0x00}},
	{0xE3,4,{0x00,0x00,0x33,0x33}},
	{0xE4,2,{0x44,0x44}},
	{0xE5,16,{0x04,0x6B,0xA0,0xA0,0x06,0x6B,0xA0,0xA0,0x08,0x6B,0xA0,0xA0,0x0A,0x6B,0xA0,0xA0}},
	{0xE6,4,{0x00,0x00,0x33,0x33}},
	{0xE7,2,{0x44,0x44}},
	{0xE8,16,{0x03,0x6B,0xA0,0xA0,0x05,0x6B,0xA0,0xA0,0x07,0x6B,0xA0,0xA0,0x09,0x6B,0xA0,0xA0}},
	{0xEB,7,{0x02,0x00,0x39,0x39,0x88,0x33,0x10}},
	{0xEC,2,{0x02,0x00}},
	{0xED,16,{0xFF,0x04,0x56,0x7F,0x89,0xF2,0xFF,0x3F,0xF3,0xFF,0x2F,0x98,0xF7,0x65,0x40,0xFF}},
	{0xFF,5,{0x77,0x01,0x00,0x00,0x00}},

	{0x11, 0,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,0,{}},// Display on
	{REGFLAG_DELAY, 100, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	
	{0x28,0, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	{0x10,0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
//	{0xFF,6, {0x77,0x01,0x00,0x00,0x11,0x18}},
//	{0xFF,5, {0x77,0x01,0x00,0x00,0x91}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    
    for(i = 0; i < count; i++) 
    {
        unsigned cmd;
        cmd = table[i].cmd;
        
        switch (cmd) 
        {
            case REGFLAG_DELAY :
            MDELAY(table[i].count);
            break;
            
            case REGFLAG_END_OF_TABLE :
            break;
            
            default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    
    params->type   = LCM_TYPE_DSI;
    
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    
    // enable tearing-free
    params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    
    #if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
    #else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
    #endif
    
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    
    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;
    
    // Video mode setting		
    params->dsi.intermediat_buffer_num = 2;
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
   params->dsi.vertical_sync_active = 8;
  params->dsi.vertical_backporch = 20;//10 16
  params->dsi.vertical_frontporch = 16;//8  20
  params->dsi.vertical_active_line = FRAME_HEIGHT;

  params->dsi.horizontal_sync_active = 10; //8
  params->dsi.horizontal_backporch = 100; //50 80
  params->dsi.horizontal_frontporch = 80; //50
  params->dsi.horizontal_active_pixel = FRAME_WIDTH;

  // Bit rate calculation
  params->dsi.PLL_CLOCK =200; //190
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
	MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(180);
    
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    
    #ifdef BUILD_LK
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 %s\n", __func__);
    #else
    printk("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 %s\n", __func__);
    #endif

}

static void lcm_suspend(void)
{
  //  SET_RESET_PIN(1);
  //  SET_RESET_PIN(0);
   // MDELAY(10);
  //  SET_RESET_PIN(1);
  //  MDELAY(20);
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
    lcm_init();
}

static int adc_read_vol(void)
{
    int adc[1];
    int data[4] ={0,0,0,0};
    int sum = 0;
    int adc_vol=0;
    int num = 0;
    //	int count = 0;
    //re_read:
    for(num=0;num<10;num++)
    {
        IMM_GetOneChannelValue(1, data, adc);
        sum+=(data[0]*100+data[1]);
    }
    adc_vol = sum/10;
#if defined(BUILD_LK)
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 test adc_vol is %d\n",adc_vol);
#else
    printk("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 test adc_vol is %d\n",adc_vol);
#endif
 if(adc_vol>=0 && adc_vol<90)
        return 1;
    else if(adc_vol>=90)
        return 2;
	else
		return -1;

}


static unsigned int lcm_compare_id(void)
{
    int array[4];
    char buffer[5];
    char id_high=0;
    char id_low=0;
    int id=0;
    int id_adc =0;
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
    SET_RESET_PIN(1);
    MDELAY(50);

    array[0] = 0x00053700;
    dsi_set_cmdq(array, 1, 1);
    
    read_reg_v2(0xa1, buffer, 3);
    id_adc = adc_read_vol();
    id_high = buffer[0];
    id_low = buffer[1];
    id = (id_high<<8) | id_low;
    
    #ifdef BUILD_LK
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 lk id_adc=%d\n", id_adc);
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 %s,buffer[0]=0x%x,buffer[1]=0x%x,id = 0x%x\n", __func__,buffer[0],buffer[1],id);
    #else
    printk("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 %s,buffer[0]=0x%x,buffer[1]=0x%x,id = 0x%x\n", __func__,buffer[0],buffer[1],id);
    #endif
	
	if(0x8802 == id&&id_adc==2)
	{
	 #ifdef BUILD_LK
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 return 1;");
    #else
    printk("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 return 1;");
    #endif
		return 1;
	}
		 #ifdef BUILD_LK
    printf("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 return 0;");
    #else
    printk("st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006 return 0;");
    #endif
	return 0;

}


LCM_DRIVER st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006_lcm_drv =
{
    .name			= "st7701s_fwvga_dsi_vdo_cmi_shengshi_j106_bopai_b006",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
