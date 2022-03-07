#include "head.h"

bitrate_version_set::bitrate_version_set(int _index, int _metric_type) {
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
	bitrate_kbps = (int*)malloc(sizeof(int) * (version_num + 1));
	//data_size = (double*)malloc(sizeof(double) * (version_num + 1));
	mean = (double*)malloc(sizeof(double) * (version_num));

	if (index == 0) { //Zecoder
		resolution[7] = 19201080; // 1920x1080
		resolution[6] = 1280720; // 1280x720
		resolution[5] = 640360; // 640x360
		resolution[4] = 640360; // 640x360
		resolution[3] = 400224; // 400x224
		resolution[2] = 400224; // 400x224
		resolution[1] = 400224; // 400x224

		bitrate_kbps[7] = 2500; //kbps
		bitrate_kbps[6] = 2000;
		bitrate_kbps[5] = 1500;
		bitrate_kbps[4] = 1000;
		bitrate_kbps[3] = 600;
		bitrate_kbps[2] = 400;
		bitrate_kbps[1] = 200;

		if (_metric_type == VMAF) {
			mean[6] = 95; //2000 
			mean[5] = 92.5; //1500
			mean[4] = 90; //1000
			mean[3] = 72; //600
			mean[2] = 60; //400
			mean[1] = 40; //200
		}
		else if (_metric_type == PSNR || _metric_type == MOS) { // https://dl.acm.org/doi/pdf/10.1145/3304109.3306231 이 논문 기반임
			mean[6] = 41.5; //2000 
			mean[5] = 40; //1500
			mean[4] = 38.5; //1000
			mean[3] = 35; //600
			mean[2] = 33.5; //400
			mean[1] = 32; //200
		}
		else if (_metric_type == SSIM) { //SSIM->MOS 변환
			//Light-weight Video Coding Based on Perceptual Video Quality for Live Streaming https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=8603274 
			mean[6] = 0.97; //2000 
			mean[5] = 0.96; //1500
			mean[4] = 0.95; //1000
			mean[3] = 0.93; //600
			mean[2] = 0.91; //400
			mean[1] = 0.87; //200
		}
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

		bitrate_kbps[11] = 35500;
		bitrate_kbps[10] = 23500;
		bitrate_kbps[9] = 13500;
		bitrate_kbps[8] = 9500;
		bitrate_kbps[7] = 6750;
		bitrate_kbps[6] = 4500;
		bitrate_kbps[5] = 4125;
		bitrate_kbps[4] = 2750;
		bitrate_kbps[3] = 1250;
		bitrate_kbps[2] = 700;
		bitrate_kbps[1] = 500;

		mean[10] = 99.9; //23500
		mean[9] = 99.5; //13500
		mean[8] = 98.6; //9500
		mean[7] = 98.4; //6750
		mean[6] = 98.2; //4500;
		mean[5] = 98; //4125;
		mean[4] = 96; //2750;
		mean[3] = 91.3; //1250;
		mean[2] = 77; //700;
		mean[1] = 65; //500;
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

		bitrate_kbps[10] = 5800;
		bitrate_kbps[9] = 4300;
		bitrate_kbps[8] = 3000;
		bitrate_kbps[7] = 2350;
		bitrate_kbps[6] = 1750;
		bitrate_kbps[5] = 1050;
		bitrate_kbps[4] = 750;
		bitrate_kbps[3] = 560;
		bitrate_kbps[2] = 375;
		bitrate_kbps[1] = 235;

		mean[9] = 98.1; //4300
		mean[8] = 97; //3000
		mean[7] = 95.5; //2350
		mean[6] = 93.8; // 1750
		mean[5] = 90.2; //1050
		mean[4] = 80; //750
		mean[3] = 67.5; //560
		mean[2] = 57.5; //375
		mean[1] = 43; // 235
	}
	else if (index == 3) {// IBM Watson Media
		resolution[7] = 38402160; // 3840x2160
		resolution[6] = 19201080; // 1920x1080
		resolution[5] = 1280720; // 1280x720
		resolution[4] = 960540; // 960x540
		resolution[3] = 854480; //  854x480
		resolution[2] = 640360; //  640x360
		resolution[1] = 480270; // 480x270

		bitrate_kbps[7] = 11000;
		bitrate_kbps[6] = 6000;
		bitrate_kbps[5] = 2750;
		bitrate_kbps[4] = 1350;
		bitrate_kbps[3] = 1350;
		bitrate_kbps[2] = 1000;
		bitrate_kbps[1] = 400;

		mean[6] = 98.3; //6000
		mean[5] = 96; // 2750
		mean[4] = 91.5; // 1350
		mean[3] = 91.5; // 1350
		mean[2] = 90; //1000
		mean[1] = 60; //400
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

		bitrate_kbps[9] = 10000;
		bitrate_kbps[8] = 6000;
		bitrate_kbps[7] = 4000;
		bitrate_kbps[6] = 3000;
		bitrate_kbps[5] = 2400;
		bitrate_kbps[4] = 1500;
		bitrate_kbps[3] = 807;
		bitrate_kbps[2] = 505;
		bitrate_kbps[1] = 253;

		mean[8] = 98.3; //6000
		mean[7] = 97.8; //4000
		mean[6] = 97; // 3000
		mean[5] = 95.5; //2400
		mean[4] = 92.5; //1500
		mean[3] = 82; //807
		mean[2] = 65; //505
		mean[1] = 45; // 253
	}

	/*for (int ver = 1; ver <= version_num; ver++) {
		data_size[ver] = ((double)bitrate[ver] * 1000) / (8 * 1024 * 1024); // MB/s 단위임
		//cout << data_size[ver];
	}*/

	number_for_bit_opration = pow(2, version_num - 3);
	set_versions_number_for_bit_opration = version_num - 2;
}

void set_version_set(bitrate_version_set* _version_set, short* _selected_set, short** _selected_ES) {
	//set 계산하기
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		_selected_set[ch] = 0;
		int set = 0;
		if (_selected_ES[ch][1] != -1)
			set = 1;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (_selected_ES[ch][ver] != -1)
				set += _version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1));
			//다 계산하고 +1할것
		}
		_selected_set[ch] = set;
	}
}

bool is_success_for_lowest_allocation(short** _selected_ES, int* _ES_count) {
	int alloc_cnt = 0;
	for (int ES = 0; ES <= NUM_OF_ES; ES++) {
		alloc_cnt += _ES_count[ES];
	}

	if (alloc_cnt < NUM_OF_CHANNEL) {
		//std::printf("alloc_cnt < NUM_OF_CHANNEL.\n");
		return false;
	}

	return true;
}