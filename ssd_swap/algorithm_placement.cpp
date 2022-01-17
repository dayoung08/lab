#include "header.h"

int rand_cnt = 0; //for generation seed in placement_random

int placement(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _method) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
		placement_num = placement_myAlgorithm(_SSD_list, _VIDEO_list);
		break;
	case PLACEMENT_RANDOM:
		placement_num = placement_random(_SSD_list, _VIDEO_list);
		break;
	}
	return placement_num;
}

int placement_myAlgorithm(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	bool is_full[NUM_OF_SSDs];
	memset(is_full, false, sizeof(is_full));

	set<pair<double, int>, greater<pair<double, int>>> video_list_with_bandwidth_sort;
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		video_list_with_bandwidth_sort.insert(make_pair(_VIDEO_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!video_list_with_bandwidth_sort.empty()) {
		int video_index = (*video_list_with_bandwidth_sort.begin()).second;
		set<pair<double, int>, greater<pair<double, int>>> ratio_of_ADWD_to_bandwidth;
		for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
			if (!is_full[ssd]) {
				double ADWD_placement = SIZE_OF_VIDEO / (_SSD_list[ssd].storage_space * _SSD_list[ssd].DWPD);
				double slope = (_SSD_list[ssd].ADWD + ADWD_placement) / (_SSD_list[ssd].maximum_bandwidth - _VIDEO_list[video_index].requested_bandwidth);
				ratio_of_ADWD_to_bandwidth.insert(make_pair(slope, ssd));
			}
		}
		int SSD_index = 0;
		while (!ratio_of_ADWD_to_bandwidth.empty()) {
			SSD_index = (*ratio_of_ADWD_to_bandwidth.begin()).second;
			if ((_SSD_list[SSD_index].storage_usage + SIZE_OF_VIDEO) > _SSD_list[SSD_index].storage_space ||
				(_SSD_list[SSD_index].bandwidth_usage + _VIDEO_list[video_index].requested_bandwidth) > _SSD_list[SSD_index].maximum_bandwidth) {
				is_full[SSD_index] = true;
				SSD_index = 0;
				ratio_of_ADWD_to_bandwidth.erase(*ratio_of_ADWD_to_bandwidth.begin());
			}
			else {
				break;
			}
		}

		if (SSD_index != 0) { // 비디오 video_index는 스토리지 SSD_index에 할당 되었음 
			allocate(_SSD_list, _VIDEO_list, video_index, SSD_index);
			placement_num++;
		}
		else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			_VIDEO_list[video_index].assigned_SSD = 0;
			_VIDEO_list[video_index].is_alloc = false;
		}
	}
}

int placement_random(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	int placement_num = 0;
	vector<int> not_full_ssd_list(NUM_OF_SSDs);
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		not_full_ssd_list[ssd - 1] = ssd;
	}

	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int SSD_index = 0;
		std::mt19937 g(SEED + rand_cnt);
		rand_cnt++;
		std::shuffle(not_full_ssd_list.begin(), not_full_ssd_list.end(), g); //랜덤으로 할당하기 위한 것. 
		while (!not_full_ssd_list.empty()) {
			SSD_index = not_full_ssd_list.back();
			if ((_SSD_list[SSD_index].storage_usage + SIZE_OF_VIDEO) > _SSD_list[SSD_index].storage_space ||
				(_SSD_list[SSD_index].bandwidth_usage + _VIDEO_list[video_index].requested_bandwidth) > _SSD_list[SSD_index].maximum_bandwidth) {
				SSD_index = 0;
				not_full_ssd_list.pop_back();
			}
			else {
				break;
			}
		}

		if (SSD_index != 0) {  // 비디오 video_index는 스토리지 SSD_index에 할당 되었음 
			allocate(_SSD_list, _VIDEO_list, video_index, SSD_index);
			placement_num++;
		}
		else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			_VIDEO_list[video_index].assigned_SSD = 0;
			_VIDEO_list[video_index].is_alloc = false;
		}
	}
}

void allocate(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _video_index, int _SSD_index) {
	_VIDEO_list[_video_index].assigned_SSD = _SSD_index;
	_SSD_list[_SSD_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_video_index].requested_bandwidth, _video_index));

	_SSD_list[_SSD_index].storage_usage += SIZE_OF_VIDEO;
	_SSD_list[_SSD_index].bandwidth_usage += _VIDEO_list[_video_index].requested_bandwidth;

	_VIDEO_list[_video_index].is_alloc = true;
}