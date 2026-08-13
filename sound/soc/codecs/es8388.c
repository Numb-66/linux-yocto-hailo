/*
 * es8388.c  --  ES8388 ALSA Soc Audio driver
 *
 *
 *
 * Author:  <everest-semi.com>
 *
 * Based on es8328.c from Everest Semiconductor
 *
 * 
 *	
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/regmap.h>

#include "es8388.h"

#define ES8388_DEBUG

#define ES8388_VERSION "v1.1"
#define es8388_DEF_VOL	0x1e //set output vol

/* codec private data */
struct es8388_priv {
	struct regmap *regmap;
	unsigned int sysclk;
	struct gpio_desc *spk_gpio;	/* SPK_CTL_H: external speaker-amp enable */
};

/*
 * es8388 register
 */
static struct reg_default  es8388_reg_defaults[] = {
	{ 0,  0x06 },
	{ 1,  0x1c },
	{ 2,  0xC0 },
	{ 3,  0xFC },
	{ 4,  0x3C },
	{ 5,  0xFF },
	{ 6,  0x01 },
	{ 7,  0x7C },
	{ 8,  0x80 },
	{ 9,  0x00 },
	{ 10, 0x54 },//00 -> 54
	//{ 10, 0x00 },//00 -> 54
	{ 11, 0x8e }, //06 -> 8e
	//{ 11, 0x06 }, //06 -> 8e
	{ 12, 0x00 },
	{ 13, 0x08 },//06 -> 08 
	{ 14, 0x30 },
	{ 15, 0x30 },
	{ 16, 0xC0 },
	{ 17, 0xC0 },
	{ 18, 0x38 },
	{ 19, 0xB0 },
	{ 20, 0x32 },
	{ 21, 0x06 },
	{ 22, 0x00 },
	{ 23, 0x00 },
	{ 24, 0x08 },//06 -> 08 
	//{ 24, 0x06 },//06 -> 08 
	{ 25, 0x02 },
	{ 26, 0x00 },
	{ 27, 0x00 },
	{ 28, 0x08 },
	{ 29, 0x06 },
	{ 30, 0x1F },
	{ 31, 0xF7 },
	{ 32, 0xFD },
	{ 33, 0xFF },
	{ 34, 0x1F },
	{ 35, 0xF7 },
	{ 36, 0xFD },
	{ 37, 0xFF },
	{ 38, 0x09 }, //00 -> 09
	//{ 38, 0x00 }, //00 -> 09
	{ 39, 0xB8 },
	{ 40, 0xB8 },
	{ 41, 0xB8 },
	{ 42, 0xB8 },
	{ 43, 0x38 },
	{ 44, 0x38 },
	{ 45, 0x00 },
	{ 46, 0x40 },
	{ 47, 0x40 },
	{ 48, 0x40 },
	{ 49, 0x40 },
	{ 50, 0x00 },
	{ 51, 0x00 },
	{ 52, 0x00 },
};

static const char *es8388_line_texts[] = {
	"Line 1", "Line 2", "PGA"};

static const unsigned int es8388_line_values[] = {
	0, 1, 3};
static const char *es8388_pga_sel[] = {"Input 1", "Input 2", "Differential"};
static const char *stereo_3d_txt[] = {"No 3D  ", "Level 1","Level 2","Level 3","Level 4","Level 5","Level 6","Level 7"};
static const char *alc_func_txt[] = {"Off", "Right", "Left", "ALCStereo"};
static const char *ng_type_txt[] = {"Constant PGA Gain","Mute ADC Output"};
static const char *deemph_txt[] = {"None", "32Khz", "44.1Khz", "48Khz"};
static const char *adcpol_txt[] = {"Normal", "L Invert", "R Invert","L + R Invert"};
static const char *es8388_mono_mux[] = {"Stereo", "Mono (Left)","Mono (Right)"};
static const char *es8388_diff_sel[] = {"Diff input 1", "Diff input 2"};
				   
static const struct soc_enum es8388_enum[]={	
	SOC_VALUE_ENUM_SINGLE(ES8388_DACCONTROL16, 3, 7, ARRAY_SIZE(es8388_line_texts), es8388_line_texts, es8388_line_values),/* LLINE */
	SOC_VALUE_ENUM_SINGLE(ES8388_DACCONTROL16, 0, 7, ARRAY_SIZE(es8388_line_texts), es8388_line_texts, es8388_line_values),/* rline	*/
	SOC_VALUE_ENUM_SINGLE(ES8388_ADCCONTROL2, 6, 3, ARRAY_SIZE(es8388_pga_sel), es8388_pga_sel, es8388_line_values),/* Left PGA Mux */
	SOC_VALUE_ENUM_SINGLE(ES8388_ADCCONTROL2, 4, 3, ARRAY_SIZE(es8388_pga_sel), es8388_pga_sel, es8388_line_values),/* Right PGA Mux */
	SOC_ENUM_SINGLE(ES8388_DACCONTROL7, 2, 8, stereo_3d_txt),/* stereo-3d */
	SOC_ENUM_SINGLE(ES8388_ADCCONTROL10, 6, 4, alc_func_txt),/*alc func*/
	SOC_ENUM_SINGLE(ES8388_ADCCONTROL14, 1, 2, ng_type_txt),/*noise gate type*/
	SOC_ENUM_SINGLE(ES8388_DACCONTROL6, 6, 4, deemph_txt),/*Playback De-emphasis*/
	SOC_ENUM_SINGLE(ES8388_ADCCONTROL6, 6, 4, adcpol_txt),
	SOC_ENUM_SINGLE(ES8388_ADCCONTROL3, 3, 3, es8388_mono_mux),
	SOC_ENUM_SINGLE(ES8388_ADCCONTROL3, 7, 2, es8388_diff_sel),
};
	
struct es8388_priv es8388_data;

static const DECLARE_TLV_DB_SCALE(pga_tlv, 0, 300, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, -9600, 50, 1);
static const DECLARE_TLV_DB_SCALE(dac_tlv, -9600, 50, 0);
static const DECLARE_TLV_DB_SCALE(out_tlv, -4500, 150, 0);
static const DECLARE_TLV_DB_SCALE(bypass_tlv, -1500, 300, 0);

#ifdef ES8388_DEBUG
static const char *test_reg_select[]   =
{
    "read ES8388 Reg 00:0x34",
	"read ES8388 Reg 00:0x34",
};

static const struct soc_enum es8388_enum_reg[] =
{
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};

static int nTestRegNo = 0;

static int get_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
    ucontrol->value.enumerated.item[0] = nTestRegNo;
    return 0;

}

static int set_test_reg(
struct snd_kcontrol       *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	u32 currMode = ucontrol->value.enumerated.item[0];
	int i, regs, rege;
	unsigned int value;

	nTestRegNo = currMode;

	switch(nTestRegNo) {
		case 1:
			//regs = 0x00;
			//rege = 0x34;
			break;
		default:
			regs = 0x00;
			rege = 0x16;
			break;
	}

	for ( i = regs ; i <= rege ; i++ ){
		value = snd_soc_component_read(component, i);
		printk("***ES8388 Addr,Reg=(%d/%x : %x)\n", i, i,value);
	}

	return(0);

}
#endif

static const struct snd_kcontrol_new es8388_snd_controls[] = {
	SOC_ENUM("3D Mode", es8388_enum[4]),
	SOC_SINGLE_TLV("L PGA", ES8388_ADCCONTROL1,
    0, 8, 0, pga_tlv),
  SOC_SINGLE_TLV("R PGA", ES8388_ADCCONTROL1,
    4, 8, 0, pga_tlv),  
    SOC_ENUM("Diffinput Set",es8388_enum[10]),
	SOC_SINGLE("ALC Capture Target Volume", ES8388_ADCCONTROL11, 4, 15, 0),
	SOC_SINGLE("ALC Capture Max PGA", ES8388_ADCCONTROL10, 3, 7, 0),
	SOC_SINGLE("ALC Capture Min PGA", ES8388_ADCCONTROL10, 0, 7, 0),
	SOC_ENUM("ALC Capture Function", es8388_enum[5]),
	SOC_SINGLE("ALC Capture ZC Switch", ES8388_ADCCONTROL13, 6, 1, 0),
	SOC_SINGLE("ALC Capture Hold Time", ES8388_ADCCONTROL11, 0, 15, 0),
	SOC_SINGLE("ALC Capture Decay Time", ES8388_ADCCONTROL12, 4, 15, 0),
	SOC_SINGLE("ALC Capture Attack Time", ES8388_ADCCONTROL12, 0, 15, 0),
	SOC_SINGLE("ALC Capture NG Threshold", ES8388_ADCCONTROL14, 3, 31, 0),
	SOC_ENUM("ALC Capture NG Type",es8388_enum[6]),
	SOC_SINGLE("ALC Capture NG Switch", ES8388_ADCCONTROL14, 0, 1, 0),
	SOC_SINGLE("ZC Timeout Switch", ES8388_ADCCONTROL13, 6, 1, 0),
	SOC_DOUBLE_R_TLV("Capture Digital Volume", ES8388_ADCCONTROL8, ES8388_ADCCONTROL9,0, 255, 1, adc_tlv),		 
	SOC_SINGLE("Capture Mute", ES8388_ADCCONTROL7, 2, 1, 0),		
	SOC_SINGLE_TLV("Left Channel Capture Volume",	ES8388_ADCCONTROL1, 4, 15, 0, bypass_tlv),
	SOC_SINGLE_TLV("Right Channel Capture Volume",	ES8388_ADCCONTROL1, 0, 15, 0, bypass_tlv),
	SOC_ENUM("Playback De-emphasis", es8388_enum[7]),
	SOC_ENUM("Capture Polarity", es8388_enum[8]),
	SOC_DOUBLE_R_TLV("PCM Volume", ES8388_DACCONTROL4, ES8388_DACCONTROL5, 0, 255, 1, dac_tlv),
	SOC_SINGLE_TLV("Left Mixer Left Bypass Volume", ES8388_DACCONTROL17, 3, 7, 1, bypass_tlv),
	SOC_SINGLE_TLV("Right Mixer Right Bypass Volume", ES8388_DACCONTROL20, 3, 7, 1, bypass_tlv),
	SOC_DOUBLE_R_TLV("Output 1 Playback Volume", ES8388_DACCONTROL24, ES8388_DACCONTROL25, 0, 64, 0, out_tlv),
	SOC_DOUBLE_R_TLV("Output 2 Playback Volume", ES8388_DACCONTROL26, ES8388_DACCONTROL27, 0, 64, 0, out_tlv),
#ifdef ES8388_DEBUG
	SOC_ENUM_EXT("Reg R", es8388_enum_reg[0], get_test_reg,set_test_reg),
#endif	
};

static const struct snd_kcontrol_new es8388_left_line_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[0]);

static const struct snd_kcontrol_new es8388_right_line_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[1]);

/* Left PGA Mux */
static const struct snd_kcontrol_new es8388_left_pga_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[2]);
/* Right PGA Mux */
static const struct snd_kcontrol_new es8388_right_pga_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[3]);

/* Left Mixer */
static const struct snd_kcontrol_new es8388_left_mixer_controls[] = {
	SOC_DAPM_SINGLE("Left Playback Switch", ES8388_DACCONTROL17, 7, 1, 0),
	SOC_DAPM_SINGLE("Left Bypass Switch", ES8388_DACCONTROL17, 6, 1, 0),	
};

/* Right Mixer */
static const struct snd_kcontrol_new es8388_right_mixer_controls[] = {
	SOC_DAPM_SINGLE("Right Playback Switch", ES8388_DACCONTROL20, 7, 1, 0),
	SOC_DAPM_SINGLE("Right Bypass Switch", ES8388_DACCONTROL20, 6, 1, 0),
};

/* Differential Mux */
//static const char *es8388_diff_sel[] = {"Line 1", "Line 2"};
static const struct snd_kcontrol_new es8388_diffmux_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[10]);

/* Mono ADC Mux */
static const struct snd_kcontrol_new es8388_monomux_controls =
	SOC_DAPM_ENUM("Route", es8388_enum[9]);

static const struct snd_soc_dapm_widget es8388_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("LINPUT1"),
	SND_SOC_DAPM_INPUT("LINPUT2"),
	SND_SOC_DAPM_INPUT("RINPUT1"),
	SND_SOC_DAPM_INPUT("RINPUT2"),
	
	SND_SOC_DAPM_MICBIAS("Mic Bias", ES8388_ADCPOWER, 3, 1),	
	
	SND_SOC_DAPM_MUX("Differential Mux", SND_SOC_NOPM, 0, 0,
		&es8388_diffmux_controls),
		
	SND_SOC_DAPM_MUX("Left ADC Mux", SND_SOC_NOPM, 0, 0,
		&es8388_monomux_controls),
	SND_SOC_DAPM_MUX("Right ADC Mux", SND_SOC_NOPM, 0, 0,
		&es8388_monomux_controls),
 
	SND_SOC_DAPM_MUX("Left PGA Mux", ES8388_ADCPOWER, 7, 1,
		&es8388_left_pga_controls),
	SND_SOC_DAPM_MUX("Right PGA Mux", ES8388_ADCPOWER, 6, 1,
		&es8388_right_pga_controls),

	SND_SOC_DAPM_MUX("Left Line Mux", SND_SOC_NOPM, 0, 0,
		&es8388_left_line_controls),
	SND_SOC_DAPM_MUX("Right Line Mux", SND_SOC_NOPM, 0, 0,
		&es8388_right_line_controls),

	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", ES8388_ADCPOWER, 4, 1),
	SND_SOC_DAPM_ADC("Left ADC", "Left Capture", ES8388_ADCPOWER, 5, 1),

	/* gModify.Cmmt Implement when suspend/startup */
	SND_SOC_DAPM_DAC("Right DAC", "Right Playback", ES8388_DACPOWER, 7, 1),
	SND_SOC_DAPM_DAC("Left DAC", "Left Playback", ES8388_DACPOWER, 6, 1),

	SND_SOC_DAPM_MIXER("Left Mixer", SND_SOC_NOPM, 0, 0,
		&es8388_left_mixer_controls[0],
		ARRAY_SIZE(es8388_left_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right Mixer", SND_SOC_NOPM, 0, 0,
		&es8388_right_mixer_controls[0],
		ARRAY_SIZE(es8388_right_mixer_controls)),

	SND_SOC_DAPM_PGA("Right Out 2", ES8388_DACPOWER, 2, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Left Out 2", ES8388_DACPOWER, 3, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Right Out 1", ES8388_DACPOWER, 4, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Left Out 1", ES8388_DACPOWER, 5, 0, NULL, 0),
	//SND_SOC_DAPM_PGA("LAMP", ES8388_ADCCONTROL1, 4, 0, NULL, 0),
	//SND_SOC_DAPM_PGA("RAMP", ES8388_ADCCONTROL1, 0, 0, NULL, 0),

	SND_SOC_DAPM_OUTPUT("LOUT1"),
	SND_SOC_DAPM_OUTPUT("ROUT1"),
	SND_SOC_DAPM_OUTPUT("LOUT2"),
	SND_SOC_DAPM_OUTPUT("ROUT2"),
	SND_SOC_DAPM_OUTPUT("VREF"),
};

static const struct snd_soc_dapm_route es8388_dapm_routes[] = {
	{"MIC1", NULL, "Mic Bias"},
  {"MIC2", NULL, "Mic Bias"},
	
	{"Differential Mux", "Diff input 1", "MIC1"},
  {"Differential Mux", "Diff input 2", "MIC2"},
	
		
	{ "Left PGA Mux", "Input 1", "LINPUT1" },
	{ "Left PGA Mux", "Input 2", "LINPUT2" },
	{ "Left PGA Mux", "Differential", "Differential Mux" },

	{ "Right PGA Mux", "Input 1", "RINPUT1" },
	{ "Right PGA Mux", "Input 2", "RINPUT2" },
	{ "Right PGA Mux", "Differential", "Differential Mux" },


	{ "Left ADC Mux", "Stereo", "Left PGA Mux" },
	{ "Left ADC Mux", "Mono (Left)", "Left PGA Mux" },


	{ "Right ADC Mux", "Stereo", "Right PGA Mux" },
	{ "Right ADC Mux", "Mono (Right)", "Right PGA Mux" },


	{ "Left ADC", NULL, "Left ADC Mux" },
	{ "Right ADC", NULL, "Right ADC Mux" },

	{ "Left Line Mux", "Line 1", "LINPUT1" },
	{ "Left Line Mux", "Line 2", "LINPUT2" },
	{ "Left Line Mux", "PGA", "Left PGA Mux" },

	{ "Right Line Mux", "Line 1", "RINPUT1" },
	{ "Right Line Mux", "Line 2", "RINPUT2" },
	{ "Right Line Mux", "PGA", "Right PGA Mux" },	

	{ "Left Mixer", "Left Playback Switch", "Left DAC" },
	{ "Left Mixer", "Left Bypass Switch", "Left Line Mux" },

	{ "Right Mixer", "Right Playback Switch", "Right DAC" },
	{ "Right Mixer", "Right Bypass Switch", "Right Line Mux" },

	{ "Left Out 1", NULL, "Left Mixer" },
	{ "LOUT1", NULL, "Left Out 1" },
	{ "Right Out 1", NULL, "Right Mixer" },
	{ "ROUT1", NULL, "Right Out 1" },

	{ "Left Out 2", NULL, "Left Mixer" },
	{ "LOUT2", NULL, "Left Out 2" },
	{ "Right Out 2", NULL, "Right Mixer" },
	{ "ROUT2", NULL, "Right Out 2" },
};


static int es8388_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	#if 1
	//struct snd_soc_codec *codec = dai->codec;
	struct snd_soc_component *component = dai->component;
	
	struct es8388_priv *es8388 = snd_soc_component_get_drvdata(component);

	es8388->sysclk = freq;

	#else
	struct snd_soc_component *component = dai->component;
	struct ak4951_priv *es8388 = snd_soc_component_get_drvdata(component);
	u32 pllpwr = 0, pll = 0;

	akdbgprt("\t[AK4951] %s(%d),CLK id is %d\n",__FUNCTION__,__LINE__, clk_id);

	snd_soc_component_read(component, AK4951_05_MODE_CONTROL1, &pll);
	akdbgprt("\t[AK4951] pll valuse is 0x%x\n",pll);
	pll &=(~0xF0);
	snd_soc_component_read(component,AK4951_01_POWER_MANAGEMENT2, &pllpwr);
	akdbgprt("\t[AK4951] pllpwr valuse is 0x%x\n",pllpwr);
	pllpwr &=(~0x0c);

	if (clk_id == AK4951_MCLK_IN) {
		pll |= AK4951_PLL_12_288MHZ;
		pllpwr &= (~AK4951_PMPLL);
		pllpwr &= (~AK4951_M_S);
	}else if (clk_id == AK4951_BCLK_IN) {
		pllpwr |= AK4951_PMPLL;
		pllpwr &= (~AK4951_M_S);
		pll |= ak4951->fmt;
	}else if (clk_id == AK4951_MCLK_IN_BCLK_OUT) {
		pllpwr |= AK4951_PMPLL;
		pllpwr |= AK4951_M_S;
		ak4951_set_pll(&pll, clk_id, freq);
	}
	snd_soc_component_write(component, AK4951_05_MODE_CONTROL1, pll);
	snd_soc_component_write(component, AK4951_01_POWER_MANAGEMENT2, pllpwr);
	msleep(5); //AKM suggested

	ak4951->sysclk = freq;
	ak4951->clkid = clk_id;


	#endif
	return 0;
}

static int es8388_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	struct snd_soc_component *component = codec_dai->component;
	u32 iface = 0;
	u32 adciface = 0;
	u32 daciface = 0;

	iface = snd_soc_component_read(component, ES8388_IFACE);
	adciface = snd_soc_component_read(component, ES8388_ADC_IFACE);
	daciface = snd_soc_component_read(component, ES8388_DAC_IFACE);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:    // MASTER MODE
		iface |= 0x80;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:    // SLAVE MODE
		iface &= 0x7F;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		adciface &= 0xFC;
		daciface &= 0xF9;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface &= 0xDF;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x20;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x20;
		adciface &= 0xDF;
		daciface &= 0xBF;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface &= 0xDF;
		adciface |= 0x20;
		daciface |= 0x40;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_component_write(component, ES8388_IFACE    , iface);
	snd_soc_component_write(component, ES8388_ADC_IFACE, adciface);
	snd_soc_component_write(component, ES8388_DAC_IFACE, daciface);

	return 0;
}

static int es8388_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	//struct snd_soc_codec *codec = dai->codec;
	struct snd_soc_component *component = dai->component;
	u16 iface;u32 tmp = 0;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		tmp = snd_soc_component_read(component, ES8388_DAC_IFACE);
		iface = tmp & 0xC7;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0018;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0008;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x0020;
			break;
		}
		/* set iface & srate */
		snd_soc_component_write(component, ES8388_DAC_IFACE, iface);
	} else {
		tmp = 0;
		tmp = snd_soc_component_read(component, ES8388_ADC_IFACE);
		iface = tmp & 0xE3;
		/* bit size */
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x000C;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x0010;
			break;
		}
		/* set iface */
		snd_soc_component_write(component, ES8388_ADC_IFACE, iface);
	}

	return 0;
}

static int es8388_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *component = dai->component;
	struct es8388_priv *es8388 = snd_soc_component_get_drvdata(component);
	unsigned int val = 0;

	val = snd_soc_component_read(component, ES8388_DAC_MUTE);
	if (mute){
		val |= 0x04;
	} else {
		val &= ~0x04;
	}

	snd_soc_component_write(component, ES8388_DAC_MUTE, val);

	/*
	 * Gate the external speaker amplifier (SPK_CTL_H) with playback:
	 * enable it only while a playback stream is running, so the amp is
	 * off when idle (no amplified codec-idle noise / buzz).
	 */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK && es8388->spk_gpio)
		gpiod_set_value_cansleep(es8388->spk_gpio, mute ? 0 : 1);

	return 0;
}

static int es8388_set_bias_level(struct snd_soc_component *component,
		enum snd_soc_bias_level level)
{

	//struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	switch(level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		//if(dapm->bias_level != SND_SOC_BIAS_ON) {
			/* updated by David-everest,5-25
			// Chip Power on
			snd_soc_write(codec, ES8388_CHIPPOWER, 0xF3);
			// VMID control
			snd_soc_write(codec, ES8388_CONTROL1 , 0x06);
			// ADC/DAC DLL power on
			snd_soc_write(codec, ES8388_CONTROL2 , 0xF3);
			*/
			snd_soc_component_write(component, ES8388_ADCPOWER, 0x00);
			snd_soc_component_write(component, ES8388_DACPOWER , 0x30);
			snd_soc_component_write(component, ES8388_CHIPPOWER , 0x00);
		//}
		break;

	case SND_SOC_BIAS_STANDBY:
		/*
		// ADC/DAC DLL power on
		snd_soc_write(codec, ES8388_CONTROL2 , 0xFF);
		// Chip Power off
		snd_soc_write(codec, ES8388_CHIPPOWER, 0xF3);
		*/
		snd_soc_component_write(component, ES8388_ADCPOWER, 0x00);
		snd_soc_component_write(component, ES8388_DACPOWER , 0x30);
		snd_soc_component_write(component, ES8388_CHIPPOWER , 0x00);
		break;

	case SND_SOC_BIAS_OFF:
		/*
		// ADC/DAC DLL power off
		snd_soc_write(codec, ES8388_CONTROL2 , 0xFF);
		// Chip Control
		snd_soc_write(codec, ES8388_CONTROL1 , 0x00);
		// Chip Power off
		snd_soc_write(codec, ES8388_CHIPPOWER, 0xFF);
		*/
		snd_soc_component_write(component, ES8388_ADCPOWER, 0xFF);
		snd_soc_component_write(component, ES8388_DACPOWER , 0xC0);
		snd_soc_component_write(component, ES8388_CHIPPOWER , 0xC3);
		break;
	}
	//dapm->bias_level = level;
	//codec->dapm.bias_level = level;

	return 0;
}


#define ES8388_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
                    SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
                    SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define ES8388_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
                    SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dai_ops es8388_dai_ops = {
	.hw_params    = es8388_pcm_hw_params,
	.set_fmt      = es8388_set_dai_fmt,
	.set_sysclk   = es8388_set_dai_sysclk,
	.mute_stream  = es8388_mute_stream,
};

struct snd_soc_dai_driver es8388_dai = {
	.name = "es8388-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ES8388_RATES,
		.formats = ES8388_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ES8388_RATES,
		.formats = ES8388_FORMATS,},
	.ops = &es8388_dai_ops,
	.symmetric_rate = 1,
};

static int es8388_suspend(struct snd_soc_component *component)
{
	es8388_set_bias_level(component, SND_SOC_BIAS_OFF);
	return 0;
}

static int es8388_resume(struct snd_soc_component *component)
{
	es8388_set_bias_level(component, SND_SOC_BIAS_STANDBY);

	return 0;
}

static void es8388_remove(struct snd_soc_component *component)
{
	es8388_set_bias_level(component, SND_SOC_BIAS_OFF);
	return ;
}

static inline int es8388_reset(struct regmap *map)
{
	int ret = 0;
	regmap_write(map, 0x00, 0x80);
	ret = regmap_write(map, 0x00, 0x00);
	return ret;
}

static int es8388_probe(struct snd_soc_component *component)
{

	// struct es8388_priv *es8388 = &es8388_data;
	int ret = 0;
	int i = 0;

	printk("es8388_probe ES8388 Audio Codec %s", ES8388_VERSION);
	//codec->control_data = es8388->regmap;	


	es8388_set_bias_level(component, SND_SOC_BIAS_STANDBY);

	snd_soc_component_write(component, ES8388_MASTERMODE,0x00);
	snd_soc_component_write(component, ES8388_CHIPPOWER, 0xF3);
	snd_soc_component_write(component, ES8388_DACCONTROL23, 0x18);//00->18
//	snd_soc_component_write(component, ES8388_ANAVOLMANAG, 0x74);
	snd_soc_component_write(component, ES8388_DACCONTROL21, 0x80);
	snd_soc_component_write(component, ES8388_CONTROL1, 0x10);
	snd_soc_component_write(component, ES8388_CHIPLOPOW2, 0xFF);
	snd_soc_component_write(component, ES8388_CHIPLOPOW1, 0x00);
	//-------------ADC---------------------------//
	snd_soc_component_write(component, ES8388_ADCCONTROL1, 0x88);
	snd_soc_component_write(component, ES8388_ADCCONTROL2, 0x50); //f0 -> 5
	snd_soc_component_write(component, ES8388_ADCCONTROL3, 0x8a); //02 -> 8a
	snd_soc_component_write(component, ES8388_ADCCONTROL4, 0x4C); //0c -> 4c
	/*
	 * ADC MCLK ratio: 0x02 = 256fs -> 48 kHz at 12.288 MHz MCLK.
	 * The Hailo15 I2S SCU clock cannot do native 8 kHz (bclk 256 kHz is
	 * rejected), so capture always runs at 48 kHz and 8 kHz production
	 * audio is downsampled in software.  (0x0a = 1536fs/8 kHz - wrong here.)
	 */
	snd_soc_component_write(component, ES8388_ADCCONTROL5, 0x02);
	snd_soc_component_write(component, ES8388_ADCCONTROL8, 0x00);
	snd_soc_component_write(component, ES8388_ADCCONTROL9, 0x00);
	snd_soc_component_write(component, ES8388_ADCCONTROL10, 0x9A); //da -> 9a
	snd_soc_component_write(component, ES8388_ADCCONTROL11, 0xB0);
	snd_soc_component_write(component, ES8388_ADCCONTROL12, 0x12);
	snd_soc_component_write(component, ES8388_ADCCONTROL13, 0x06);
	snd_soc_component_write(component, ES8388_ADCCONTROL14, 0x11);
	//-------------DAC-----------------------------//
	snd_soc_component_write(component, ES8388_DACCONTROL1, 0x18);
	snd_soc_component_write(component, ES8388_DACCONTROL2, 0x02);
	snd_soc_component_write(component, ES8388_DACCONTROL3, 0x02);//76-> 02
	snd_soc_component_write(component, ES8388_DACCONTROL4, 0x00);
	snd_soc_component_write(component, ES8388_DACCONTROL5, 0x00);
	snd_soc_component_write(component, ES8388_DACCONTROL17, 0xB8);
	snd_soc_component_write(component, ES8388_DACCONTROL20, 0xB8);

	snd_soc_component_write(component, ES8388_CHIPPOWER, 0x00);
	snd_soc_component_write(component, ES8388_CONTROL1, 0x16);
	snd_soc_component_write(component, ES8388_CONTROL2, 0x72);
	snd_soc_component_write(component, ES8388_DACPOWER, 0x3C);
	snd_soc_component_write(component, ES8388_ADCPOWER, 0x00);
	snd_soc_component_write(component, ES8388_CONTROL1, 0x16);//32-> 16
	snd_soc_component_write(component, ES8388_DACCONTROL3, 0x02);

	for( i = 0; i < es8388_DEF_VOL; i++)
	{
		snd_soc_component_write(component, ES8388_DACCONTROL24, i);    //LOUT1/ROUT1 VOLUME
		snd_soc_component_write(component, ES8388_DACCONTROL25, i);
		snd_soc_component_write(component, ES8388_DACCONTROL26, i);    //LOUT2/ROUT2 VOLUME
		snd_soc_component_write(component, ES8388_DACCONTROL27, i);
		msleep(5);
	}
	es8388_set_bias_level(component, SND_SOC_BIAS_STANDBY);
	pr_info("-%s()\n",__FUNCTION__);
	return ret;
}

static bool es8388_volatile_register(struct device *dev,
							unsigned int reg)
{
	switch (reg) {
		#if 0
		case 0:
		#else
		case ES8388_CONTROL1: 
		case ES8388_CONTROL2: 
		case ES8388_CHIPPOWER: 
		case ES8388_ADCPOWER: 
		case ES8388_DACPOWER: 
		case ES8388_CHIPLOPOW1: 
		case ES8388_CHIPLOPOW2: 
		case ES8388_ANAVOLMANAG: 
		case ES8388_MASTERMODE: 
		case ES8388_ADCCONTROL1: 
		case ES8388_ADCCONTROL2: 
		case ES8388_ADCCONTROL3: 
		case ES8388_ADCCONTROL4: 
		case ES8388_ADCCONTROL5: 
		case ES8388_ADCCONTROL6: 
		case ES8388_ADCCONTROL7: 
		case ES8388_ADCCONTROL8: 
		case ES8388_ADCCONTROL9: 
		case ES8388_ADCCONTROL10: 
		case ES8388_ADCCONTROL11: 
		case ES8388_ADCCONTROL12: 
		case ES8388_ADCCONTROL13: 
		case ES8388_ADCCONTROL14: 				
		case ES8388_DACCONTROL1: 
		case ES8388_DACCONTROL2: 
		case ES8388_DACCONTROL3: 
		case ES8388_DACCONTROL4: 
		case ES8388_DACCONTROL5: 
		case ES8388_DACCONTROL6: 
		case ES8388_DACCONTROL7: 
		case ES8388_DACCONTROL8: 
		case ES8388_DACCONTROL9: 
		case ES8388_DACCONTROL10: 
		case ES8388_DACCONTROL11: 
		case ES8388_DACCONTROL12: 
		case ES8388_DACCONTROL13: 
		case ES8388_DACCONTROL14: 
		case ES8388_DACCONTROL15: 
		case ES8388_DACCONTROL16: 
		case ES8388_DACCONTROL17: 
		case ES8388_DACCONTROL18: 
		case ES8388_DACCONTROL19: 
		case ES8388_DACCONTROL20: 
		case ES8388_DACCONTROL21: 
		case ES8388_DACCONTROL22: 
		case ES8388_DACCONTROL23: 
		case ES8388_DACCONTROL24: 
		case ES8388_DACCONTROL25: 
		case ES8388_DACCONTROL26: 
		case ES8388_DACCONTROL27: 
		case ES8388_DACCONTROL28: 
		case ES8388_DACCONTROL29: 
		case ES8388_DACCONTROL30:
		#endif
		return true;

	default:
		break;
	}

	return false;
}



static struct snd_soc_component_driver soc_codec_dev_es8388 = {
	.probe			= es8388_probe,
	.remove 		= es8388_remove,
	.suspend		= es8388_suspend,
	.resume 		= es8388_resume,
	.set_bias_level 	= es8388_set_bias_level,
	.controls		= es8388_snd_controls,
	.num_controls		= ARRAY_SIZE(es8388_snd_controls),
	.dapm_widgets		= es8388_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(es8388_dapm_widgets),
	.dapm_routes		= es8388_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(es8388_dapm_routes),
	
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness 	= 1,
	.non_legacy_dai_naming	= 1,
};

static struct regmap_config es8388_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = ES8388_MAX_REGISTERS,
	.reg_defaults = es8388_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(es8388_reg_defaults),
	.volatile_reg = es8388_volatile_register,
	.cache_type = REGCACHE_RBTREE,
};

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static int es8388_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct es8388_priv *es8388;
	int ret = 0;

	printk("ES8388 Audio Codec %s", ES8388_VERSION);


	es8388 = devm_kzalloc(&i2c->dev, sizeof(struct es8388_priv), GFP_KERNEL);
	if (es8388 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, es8388);
	es8388->regmap = devm_regmap_init_i2c(i2c, &es8388_regmap);
	if (IS_ERR(es8388->regmap)) {
		ret = PTR_ERR(es8388->regmap);
		dev_err(&i2c->dev, "regmap_init() failed: %d\n", ret);
		return ret;
	}

	/* Optional external speaker-amp enable (SPK_CTL_H); starts OFF/low. */
	es8388->spk_gpio = devm_gpiod_get_optional(&i2c->dev, "spk-ctl",
						   GPIOD_OUT_LOW);
	if (IS_ERR(es8388->spk_gpio)) {
		ret = PTR_ERR(es8388->spk_gpio);
		dev_err(&i2c->dev, "failed to get spk-ctl gpio: %d\n", ret);
		return ret;
	}

	ret = snd_soc_register_component(&i2c->dev,
			&soc_codec_dev_es8388, &es8388_dai, 1);
	


	return ret;
}

static int es8388_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_component(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static struct of_device_id es8388_of_match[] = {
	{ .compatible = "everest,es8388",},
	{ .compatible = "ambarella,es8388",},
	
	{},
};
MODULE_DEVICE_TABLE(of, es8388_of_match);
static const struct i2c_device_id es8388_i2c_id[] = {
	{ "es8388", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, es8388_i2c_id);

static struct i2c_driver es8388_i2c_driver = {
	.driver = {
		.name = "es8388-codec",
		.owner = THIS_MODULE,
		.of_match_table = es8388_of_match,
	},
	.probe    = es8388_i2c_probe,
	.remove   = es8388_i2c_remove,
	.id_table = es8388_i2c_id,
};
#endif

static int __init es8388_modinit(void)
{
	int ret = 0;
	printk("es8388_modinit start ff.\n");
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&es8388_i2c_driver);
	if (ret != 0) {
		printk("Failed to register ES8388 I2C driver: %d\n", ret);
	}
	printk("es8388_modinit i2c ok ff.\n");
#endif
	printk("es8388_modinit done ok ff.\n");

	return ret;
}
module_init(es8388_modinit);

static void __exit es8388_exit(void)
{
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&es8388_i2c_driver);
#endif
}
module_exit(es8388_exit);

MODULE_DESCRIPTION("ASoC ES8388 driver");
MODULE_AUTHOR("David@everest-semi.com>");
MODULE_LICENSE("GPL");

