# liblhdc-collections
Collected lhdc libs from any platform.

## Savitech LHDC Codec for AOSP
LHDC（包含LHDCV5）是[Savitech LHDC](https://lhdc.co/cn)发行的编解码技术，其最新开源内容可访问[
Savitech LHDC Codec for AOSP](https://gitlab.com/savitech-lhdc)获取。LHDC是用于最高质量无线音频的下一代技术。 LHDC 能大幅降低无线与有线音频设备之间的音频质量差异，提供最逼真的超高音质，让用户能尽情享受蓝牙无线音频带来的便利性和高质量。适用于视频、音乐和游戏等所有应用。以漫步者花再 Halo Space 体验为例，最新的LHDCV5(192kHz & 24bit)在实际体验上已经优于LDAC(96kHz & 32bit)。

比较有意思的是，包括已知的LDAC、AAC、OPUS、LC3、aptX[L/HD]在内，大多数优于SBC的A2DP codec算法都已开源，唯独LHDC还未实现彻底开源；要实现LHDC编解码，需要使用独立于项目的第三方库；大多数耳机用的是Savitech LHDC的".a"格式的静态库，而Android用的是Savitech LHDC的".so"格式的动态库；“.a”静态库通常伴随着BES等项目的SDK提供，“.so”动态库在支持LHDC的手机中可轻易找到。

## 本仓库目录介绍
- 目录**BES-IHC**，适用于BES的LHDC[V5]库，包含.a静态库和头文件，完整SDK详见[audio_prj_collections](https://github.com/sprlightning/audio_prj_collections):
	```c
	├─a2dp_decoder
	│      a2dp_decoder_lhdc.cpp
	│      a2dp_decoder_lhdcv5.cpp
	│
	├─audio_codec
	│  ├─liblhdc-dec
	│  │      BEST2500P_libLHDC_V2_V3_V4_4_0_13_SAVI_KEYPRO_UUID.a
	│  │
	│  └─liblhdcv5-dec
	│          BEST2500P_libLHDC_V5_5_2_0_SAVI_KEYPRO_UUID.a
	│
	└─audio_codec_lib
		├─liblhdc-dec
		│  │  Makefile
		│  │
		│  └─inc
		│          lhdcUtil.h
		│
		├─liblhdc-enc
		│  │  Makefile
		│  │
		│  └─inc
		│          lhdc_cfg.h
		│          lhdc_enc_api.h
		│
		└─liblhdcv5-dec
			│  Makefile
			│
			└─inc
					lhdcv5_util_dec.h
	```

- 目录**AOSP**，适用于AOSP的的LHDC[V5]库，就decoder而言，除了无解码算法源文件（lhdcv5_util_dec.c），其余源文件及头文件是完整的；此外也包含提取自XIAOMI HyperOS 2.0.211.0的".so"动态库：
	```c
	├─liblhdc
	│  │  Android.bp
	│  │  release_note
	│  │
	│  ├─inc
	│  │      lhdcBT.h
	│  │
	│  ├─include
	│  │      cirbuf.h
	│  │      lhdcv2_process.h
	│  │      lhdcv3_process.h
	│  │      lhdc_api.h
	│  │      lhdc_cfg.h
	│  │      lhdc_enc_config.h
	│  │      lhdc_process.h
	│  │      llac_enc_api.h
	│  │
	│  └─src
	│          lhdcBT_enc.c
	│
	├─liblhdc-hyperos2_0_211_0
	│  └─system
	│      └─lib64
	│              liblhdc.so
	│              liblhdcv5.so
	│
	├─liblhdcdec
	│  │  Android.bp
	│  │  release_note
	│  │
	│  ├─inc
	│  │      lhdcBT_dec.h
	│  │
	│  ├─include
	│  │      lhdcUtil.h
	│  │
	│  └─src
	│          lhdcBT_dec.c
	│
	├─liblhdcv5
	│  │  Android.bp
	│  │  release_note
	│  │
	│  ├─inc
	│  │      lhdcv5BT.h
	│  │
	│  ├─include
	│  │      lhdcv5BT_ext_func.h
	│  │      lhdcv5_api.h
	│  │
	│  └─src
	│          lhdcv5BT_enc.c
	│
	└─liblhdcv5dec
		│  Android.bp
		│  release_note
		│
		├─inc
		│      lhdcv5BT_dec.h
		│
		├─include
		│      lhdcv5_util_dec.h
		│
		└─src
				lhdcv5BT_dec.c
	```

- 目录**ESP-IDF**，包含适用于ESP-IDF的移植版[liblhdcv5dec](https://github.com/sprlightning/liblhdcv5dec)和lhdcv5 decoder，均具备完整的源文件和头文件，均移植于AOSP，使用LHDCV5协商后，可听到正弦波生成的标准音；其中lhdcv5_util_dec.c仅具备模拟解码的能力，仅供参考，用真正的LHDCV5解码算法替换其中的正弦波（模拟解码）部分可实现完整的LHDCV5音频Sink。下面是目录结构：
	```c
	└─bluedroid
		├─api
		│  └─include
		│          esp_a2dp_api.h
		│
		├─external
		│  └─liblhdcv5dec
		│      │  CMakeLists.txt
		│      │  release_note
		│      │
		│      ├─inc
		│      │      lhdcv5BT_dec.h
		│      │
		│      ├─include
		│      │      lhdcv5_util_dec.h
		│      │
		│      └─src
		│              lhdcv5BT_dec.c
		│              lhdcv5_util_dec.c
		│
		└─stack
			├─a2dp
			│      a2dp_vendor.c
			│      a2dp_vendor_lhdcv5.c
			│      a2dp_vendor_lhdcv5_decoder.c
			│
			└─include
				└─stack
						a2dp_vendor.h
						a2dp_vendor_lhdcv5.h
						a2dp_vendor_lhdcv5_constants.h
						a2dp_vendor_lhdcv5_decoder.h
						a2dp_vendor_lhdc_constants.h
	```

## LHDCV5移植
我对BES的项目不是很了解，因为这方面资料不完整；而AOSP方面资料倒是挺多的。

早段时间我从AOSP移植了LHDCV5到ESP-IDF，因为不知道LHDCV5解码算法，所以只是用正弦波替换了解码函数，ESP32作为A2DP Sink连接手机且使用LHDCV5协商后，手机播放音乐时Sink端听到的是固定的标准音，证明移植成功了。ESP-IDF默认用的是SBC，其实它还额外支持AAC（M12、M24），忘了是5.1.6还是哪个版本的ESP-IDF，其已经内置了A2DP拓展逻辑，能加入其它codecs，不过一直没人做这方面的具体拓展。我注意到cfint对ESP-IDF 5.1.4写了一套较为成熟的A2DP拓展逻辑（5.1.4本身不具备拓展能力），能依据CIE结构体在协商时动态调用对应的解码器来解码A2DP数据包，所以我就顺着他的路走下去了。大致就是1）改CIE结构体、2）在拓展的A2DP函数中增加LHDCV5函数，这方面依葫芦画瓢模仿LDAC可实现。麻烦的是3）LHDCV5第三方库的移植与实现。

1）CIE结构体：ESP-IDF的bluedroid-stack有一个存储codec能力的CIE结构体（位于esp_a2dp_api.h），这里可加入LHDCV5的CIE_LEN，可与其他设备进行A2DP协商；因为大部分CIE_LEN = CODEC_LEN - 2，而LHDCV5的CODEC_LEN查询a2dp_vendor_lhdc_constants.h可知是13，所以LHDCV5的CIE_LEN其长度是11，这已经验证过了是对的；

2）ESP-IDF的LHDCV5函数文件移植：为ESP-IDF添加LHDCV5的decoder函数，这方面参考AOSP，移植内容包括4个头文件（a2dp_vendor_lhdc_constants.h、a2dp_vendor_lhdcv5_constants.h、a2dp_vendor_lhdcv5_decoder.h、a2dp_vendor_lhdcv5.h）和个源文件（a2dp_vendor_lhdcv5_decoder.c、a2dp_vendor_lhdcv5.c），移植的内容无非是更改日志打印、内存函数、变量定义，其它大体上差不多不用动；

函数依赖方面是这样：a2d_sbc.c(修改版) --调用--> a2d_sbc_decoder.c --调用--> a2dp_vendor.c --调用--> a2dp_vendor_lhdcv5.c --调用--> a2dp_vendor_lhdcv5_decoder.c --调用--> lhdcv5BT_dec.c(外部) --调用--> lhdcv5_util_dec.c(外部)。

3）LHDCV5外部库：让我头疼的就是LHDCV5的外部库，即使是高度开源的AOSP，也只得到了lhdcv5BT_dec.c/.h + lhdcv5_util_dec.h这3个文件，缺乏包含LHDCV5解码算法的lhdcv5_util_dec.c；可以依据lhdcv5_util_dec.h声明函数的参数逆推出lhdcv5_util_dec.c，但是如前面所说，因为不知道LHDCV5解码算法，所以我只是用正弦波替换了解码函数，当然连接后听到的也只是正弦波生成的标准音。了解到的一个思路是用（IDA Pro）逆向Android LHDCV5动态库（liblhdcv5.so）或BES的静态库来推导出LHDCV5的解码算法（我不知道，我不会，我很懵，希望有大佬能在这方面指点指点QWQ...），还有就是等待大佬开源~

## LHDCV5解码算法
LHDCV5的解码算法位于lhdcv5_util_dec.c，其中真正的解码算法似乎还没人开源，这里等待大佬开源~

我这里开源那个正弦波代替解码的lhdcv5_util_dec.c（归类在ESP-IDF里），仅供参考，可用来验证你的LHDCV5移植是否成功。当然如果你搞出LHDCV5算法了，欢迎联系我，teach me! 😍😍😍我已经迫不及待想在ESP32或者SF32等设备上听到LHDCV5传输的音乐了~😍😍😍
