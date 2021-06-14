#include "head.h"

bitrate_version_set::bitrate_version_set(int _index) {
	index = _index;
	if (index == 0) { //Zecoder
		version_num = 7;
	}
	else if (index == 1) { // Youtube
		version_num = 11;
	}
	else if (index == 2) { // Neflix
		version_num = 10;
	}
	else if (index == 4) { // 논문 https://doi.org/10.1145/3123266.3123426
		version_num = 9;
	}
	else if (index == 3) { // IBM Watson Media
		version_num = 7;
	}

	version_set_num = pow(2, (version_num - 2));
	resolution = (int*)malloc(sizeof(int) * (version_num + 1));
	bitrate = (int*)malloc(sizeof(int) * (version_num + 1));
	data_size = (double*)malloc(sizeof(double) * (version_num + 1));

	if (index == 0) { //Zecoder
		resolution[7] = 19201080; // 1920x1080
		resolution[6] = 1280720; // 1280x720
		resolution[5] = 640360; // 640x360
		resolution[4] = 640360; // 640x360
		resolution[3] = 400224; // 400x224
		resolution[2] = 400224; // 400x224
		resolution[1] = 400224; // 400x224

		bitrate[7] = 2500; //kbps
		bitrate[6] = 2000;
		bitrate[5] = 1500;
		bitrate[4] = 1000;
		bitrate[3] = 600;
		bitrate[2] = 400;
		bitrate[1] = 200;
	}
	else if (index == 1) { // Youtube https://support.google.com/youtube/answer/2853702?hl=ko 기준. 비트레이트는 mean 사용.
		resolution[11] = 38402160; // 3840x2160
		resolution[10] = 38402160; // 3840x2160
		resolution[9] = 25601440; // 2560x1440
		resolution[8] = 25601440; // 2560x1440
		resolution[7] = 19201080; // 1920x1080
		resolution[6] = 19201080; // 1920x1080
		resolution[5] = 1280720; // 1280x720
		resolution[4] = 1280720; // 1280x720
		resolution[3] = 854480; //  854x480
		resolution[2] = 640360; //  640x360
		resolution[1] = 426240; //  426x240

		bitrate[11] = 35500;
		bitrate[10] = 23500;
		bitrate[9] = 13500;
		bitrate[8] = 9500;
		bitrate[7] = 6750;
		bitrate[6] = 4500;
		bitrate[5] = 4125;
		bitrate[4] = 2750;
		bitrate[3] = 1250;
		bitrate[2] = 700;
		bitrate[1] = 500;
	}
	else if (index == 2) { // Netflix https://netflixtechblog.com/per-title-encode-optimization-7e99442b62a2
		resolution[10] = 19201080; // 1920x1080
		resolution[9] = 19201080; // 1920x1080
		resolution[8] = 1280720; // 1280x720
		resolution[7] = 1280720; // 1280x720
		resolution[6] = 720480; // 720x480
		resolution[5] = 640480; // 640x480
		resolution[4] = 512384; // 512x384
		resolution[3] = 512384; // 512x384
		resolution[2] = 384288; // 384x288
		resolution[1] = 320240; // 320x240

		bitrate[10] = 5800;
		bitrate[9] = 4300;
		bitrate[8] = 3000;
		bitrate[7] = 2350;
		bitrate[6] = 1750;
		bitrate[5] = 1050;
		bitrate[4] = 750;
		bitrate[3] = 560;
		bitrate[2] = 375;
		bitrate[1] = 235;
	}
	else if (index == 4) {  // 논문 https://doi.org/10.1145/3123266.3123426
		resolution[9] = 19201080; // 1920x1080
		resolution[8] = 19201080; // 1920x1080
		resolution[7] = 19201080; // 1920x1080
		resolution[6] = 19201080; // 1920x1080
		resolution[5] = 1280720; // 1280x720
		resolution[4] = 1280720; // 1280x720
		resolution[3] = 640360; //  640x360
		resolution[2] = 640360; //  640x360
		resolution[1] = 480270; // 480x270

		bitrate[9] = 10000;
		bitrate[8] = 6000;
		bitrate[7] = 4000;
		bitrate[6] = 3000;
		bitrate[5] = 2400;
		bitrate[4] = 1500;
		bitrate[3] = 807;
		bitrate[2] = 505;
		bitrate[1] = 253;
	}
	else if (index == 3) {// IBM Watson Media
		resolution[7] = 38402160; // 3840x2160
		resolution[6] = 19201080; // 1920x1080
		resolution[5] = 1280720; // 1280x720
		resolution[4] = 960540; // 960x540
		resolution[3] = 854480; //  854x480
		resolution[2] = 640360; //  640x360
		resolution[1] = 480270; // 480x270

		bitrate[7] = 11000;
		bitrate[6] = 6000;
		bitrate[5] = 2750;
		bitrate[4] = 1350;
		bitrate[3] = 1350;
		bitrate[2] = 1000;
		bitrate[1] = 400;
	}

	for (int ver = 1; ver <= version_num; ver++) {
		data_size[ver] = ((double)bitrate[ver] * 1000) / (8 * 1024 * 1024); // MB/s 단위임
		//cout << data_size[ver];
	}
	
	number_for_bit_opration = pow(2, version_num - 3);
	set_versions_number_for_bit_opration = version_num - 2;
}

void set_version_set(bitrate_version_set* _version_set, short* _selected_set, short** _selected_ES) {
	//set 계산하기
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		_selected_set[ch] = 0;
		int set = 1;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (_selected_ES[ch][ver] != -1)
				set += _version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1));
			//다 계산하고 +1할것
		}
		_selected_set[ch] = set;
	}
}