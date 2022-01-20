#include "header.h"

int rand_cnt_for_placement = 0; //for generation seed in placement_random
int placement(SSD* _SSD_list, VIDEO* _VIDEO_list, int _method) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
		placement_num = placement_myAlgorithm(_SSD_list, _VIDEO_list);
		break;
	case PLACEMENT_BANDWIDTH_AWARE:
		placement_num = placement_bandwidth_aware(_SSD_list, _VIDEO_list);
		break;
	case PLACEMENT_RANDOM:
		placement_num = placement_random(_SSD_list, _VIDEO_list);
		break;
	}
	return placement_num;
}

int placement_myAlgorithm(SSD* _SSD_list, VIDEO* _VIDEO_list) {
	set<pair<double, int>, greater<pair<double, int>>> video_list_with_bandwidth_sort;
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		video_list_with_bandwidth_sort.insert(make_pair(_VIDEO_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!video_list_with_bandwidth_sort.empty()) {
		int video_index = (*video_list_with_bandwidth_sort.begin()).second;
		video_list_with_bandwidth_sort.erase(*video_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_ratio_sort;
		for (int ssd_temp = 1; ssd_temp <= NUM_OF_SSDs; ssd_temp++) {
			if (!is_not_enough_storage_space(_SSD_list, _VIDEO_list, ssd_temp, video_index) &&
				(_SSD_list[ssd_temp].bandwidth_usage + _VIDEO_list[video_index].requested_bandwidth) <= _SSD_list[ssd_temp].maximum_bandwidth) {
				double ADWD_placement = _VIDEO_list[video_index].size / (_SSD_list[ssd_temp].storage_space * _SSD_list[ssd_temp].DWPD);
				double slope = (_SSD_list[ssd_temp].ADWD + ADWD_placement) / (_SSD_list[ssd_temp].maximum_bandwidth - _VIDEO_list[video_index].requested_bandwidth);
				target_ssd_list_with_ratio_sort.insert(make_pair(slope, ssd_temp));
			}
		}
		//할당 가능한 SSD의 ADWD/bandwidth 값만 list에 넣음.

		if (!target_ssd_list_with_ratio_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_ratio_sort.begin()).second;
			allocate(_SSD_list, _VIDEO_list, video_index, ssd_index);
			placement_num++;
		}
		else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_list[video_index].is_alloc = false;
		}
	}
	return placement_num;
}

int placement_bandwidth_aware(SSD* _SSD_list, VIDEO* _VIDEO_list) {
	set<pair<double, int>, greater<pair<double, int>>> video_list_with_bandwidth_sort;
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		video_list_with_bandwidth_sort.insert(make_pair(_VIDEO_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!video_list_with_bandwidth_sort.empty()) {
		int video_index = (*video_list_with_bandwidth_sort.begin()).second;
		video_list_with_bandwidth_sort.erase(*video_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_bandwidth_sort;
		for (int ssd_temp = 1; ssd_temp <= NUM_OF_SSDs; ssd_temp++) {
			if (!is_not_enough_storage_space(_SSD_list, _VIDEO_list, ssd_temp, video_index) &&
				(_SSD_list[ssd_temp].bandwidth_usage + _VIDEO_list[video_index].requested_bandwidth) <= _SSD_list[ssd_temp].maximum_bandwidth) {
				target_ssd_list_with_bandwidth_sort.insert(make_pair((_SSD_list[ssd_temp].maximum_bandwidth - _VIDEO_list[video_index].requested_bandwidth), ssd_temp));
			}
		}
		//할당 가능한 SSD의 bandwidth 값만 list에 넣음.

		if (!target_ssd_list_with_bandwidth_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_bandwidth_sort.begin()).second;
			allocate(_SSD_list, _VIDEO_list, video_index, ssd_index);
			placement_num++;
		}
		else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_list[video_index].is_alloc = false;
		}
	}
	return placement_num;
}

int placement_random(SSD* _SSD_list, VIDEO* _VIDEO_list) { // 밴드윗 생각 전혀 없음
	int placement_num = 0;
	std::mt19937 g(SEED + rand_cnt_for_placement);
	rand_cnt_for_placement++;
	vector<int> target_ssd_list(NUM_OF_SSDs);
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		target_ssd_list[ssd - 1] = ssd;
	}
	std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);

	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int ssd_index = NONE_ALLOC;

		while (!target_ssd_list.empty()) {
			ssd_index = target_ssd_list.back();
			if (is_not_enough_storage_space(_SSD_list, _VIDEO_list, ssd_index, video_index))
				target_ssd_list.pop_back();
			else {
				allocate(_SSD_list, _VIDEO_list, video_index, ssd_index);
				placement_num++;
				break;
			}
		}
	}
	return placement_num;
}

void allocate(SSD* _SSD_list, VIDEO* _VIDEO_list, int _video_index, int _ssd_index) {
	_VIDEO_list[_video_index].assigned_SSD = _ssd_index;
	_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_video_index].requested_bandwidth, _video_index));

	_SSD_list[_ssd_index].storage_usage += _VIDEO_list[_video_index].size;
	_SSD_list[_ssd_index].bandwidth_usage += _VIDEO_list[_video_index].requested_bandwidth;

	_VIDEO_list[_video_index].is_alloc = true;
}