#include "header.h"

int rand_cnt_for_placement = 0; //for generation seed in placement_random
int placement(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _method, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
		placement_num = placement_myAlgorithm(_SSD_list, _existed_VIDEO_SEGMENT_list, _new_VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_existed_videos, _num_of_new_videos);
		break;
	case PLACEMENT_BANDWIDTH_AWARE:
		placement_num = placement_bandwidth_aware(_SSD_list, _new_VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_new_videos);
		break;
	case PLACEMENT_RANDOM:
		placement_num = placement_random(_SSD_list, _new_VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_new_videos);
		break;
	}

	return placement_num;
}

int placement_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos){
	set<pair<double, int>, greater<pair<double, int>>> VIDEO_SEGMENT_list_with_bandwidth_sort;
	for (int vid = 0; vid < _num_of_new_videos; vid++) {
		VIDEO_SEGMENT_list_with_bandwidth_sort.insert(make_pair(_new_VIDEO_SEGMENT_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!VIDEO_SEGMENT_list_with_bandwidth_sort.empty()) {
		int video_index = (*VIDEO_SEGMENT_list_with_bandwidth_sort.begin()).second;
		VIDEO_SEGMENT_list_with_bandwidth_sort.erase(*VIDEO_SEGMENT_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_ratio_sort;

		for (int ssd_temp = 0; ssd_temp < _num_of_SSDs; ssd_temp++) {
			bool is_removed = false;
			if (is_not_enough_storage_space(_SSD_list, _new_VIDEO_SEGMENT_list, ssd_temp, video_index)) {
				is_removed = true;
			}
			double removed_bandwidth = 0;
			if (is_removed) {
				int curr_vid = (*_SSD_list[ssd_temp].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				removed_bandwidth = _existed_VIDEO_SEGMENT_list[curr_vid].requested_bandwidth;
			}

			if ((_SSD_list[ssd_temp].bandwidth_usage - removed_bandwidth + _new_VIDEO_SEGMENT_list[video_index].requested_bandwidth) <= _SSD_list[ssd_temp].maximum_bandwidth &&
				((_SSD_list[ssd_temp].total_write_MB + _new_VIDEO_SEGMENT_list[video_index].size) / (_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD) / _SSD_list[ssd_temp].running_days) <= AVR_ADWD_LIMIT) {

				double ADWD_placement = _SSD_list[ssd_temp].ADWD + (_new_VIDEO_SEGMENT_list[video_index].size / (_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD));
				double remained_bandwidth = (_SSD_list[ssd_temp].maximum_bandwidth - (_SSD_list[ssd_temp].bandwidth_usage + _new_VIDEO_SEGMENT_list[video_index].requested_bandwidth));
				//double ss = (_SSD_list[ssd_temp].storage_capacity - (_SSD_list[ssd_temp].storage_usage + _VIDEO_SEGMENT_list[video_index].size)); 
				//remained_bandwidth / ss =남은 밴드윗/공간

				double remained_write_MB = (_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD) - (_SSD_list[ssd_temp].daily_write_MB + _new_VIDEO_SEGMENT_list[video_index].size);
				//double slope = bb / (ss * _SSD_list[ssd_temp].DWPD); //  남은 수명 대비 남은 밴드윗
				double slope = remained_bandwidth / remained_write_MB; //  남은 수명 대비 남은 밴드윗
				target_ssd_list_with_ratio_sort.insert(make_pair(slope, ssd_temp));
			}
		}

		if (!target_ssd_list_with_ratio_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_ratio_sort.begin()).second;
			allocate(_SSD_list, _existed_VIDEO_SEGMENT_list, _new_VIDEO_SEGMENT_list, PLACEMENT_OURS, ssd_index, video_index);
			placement_num++;
		}
		/*else {
			for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
				printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] storage %.2f  / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_capacity, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_capacity));
				printf("[SSD %d] Average ADWD %.2f \n", ssd, (_SSD_list[ssd].total_write_MB) / (_SSD_list[ssd].storage_capacity * _SSD_list[ssd].DWPD) / _SSD_list[ssd].running_days);
			}
			//printf("%lf\n", _VIDEO_SEGMENT_list[video_index].requested_bandwidth);
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
		}*/
	}
	return placement_num;
}

int placement_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos) {
	set<pair<double, int>, greater<pair<double, int>>> VIDEO_SEGMENT_list_with_bandwidth_sort;
	for (int vid = 0; vid < _num_of_videos; vid++) {
		VIDEO_SEGMENT_list_with_bandwidth_sort.insert(make_pair(_VIDEO_SEGMENT_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!VIDEO_SEGMENT_list_with_bandwidth_sort.empty()) {
		int video_index = (*VIDEO_SEGMENT_list_with_bandwidth_sort.begin()).second;
		VIDEO_SEGMENT_list_with_bandwidth_sort.erase(*VIDEO_SEGMENT_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_bandwidth_sort;
		for (int ssd_temp = 0; ssd_temp < _num_of_SSDs; ssd_temp++) {
			if (!is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, ssd_temp, video_index) &&
				(_SSD_list[ssd_temp].bandwidth_usage + _VIDEO_SEGMENT_list[video_index].requested_bandwidth) <= _SSD_list[ssd_temp].maximum_bandwidth) {
				//&& ((_SSD_list[ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[video_index].size) / (_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD) / _SSD_list[ssd_temp].running_days) <= AVR_ADWD_LIMIT) {

				double slope = (_SSD_list[ssd_temp].maximum_bandwidth - (_SSD_list[ssd_temp].bandwidth_usage + _VIDEO_SEGMENT_list[video_index].requested_bandwidth)) / (_SSD_list[ssd_temp].storage_capacity - (_SSD_list[ssd_temp].storage_usage + _VIDEO_SEGMENT_list[video_index].size));
				target_ssd_list_with_bandwidth_sort.insert(make_pair(slope, ssd_temp));
			}
		}
		//할당 가능한 SSD의 bandwidth 값만 list에 넣음.

		if (!target_ssd_list_with_bandwidth_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_bandwidth_sort.begin()).second;
			allocate(_SSD_list, NULL, _VIDEO_SEGMENT_list, PLACEMENT_BANDWIDTH_AWARE, ssd_index, video_index);
			placement_num++;
		}
		/*else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_SEGMENT_list[video_index].is_alloc = false;
		}*/
	}
	return placement_num;
}

int placement_random(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos) {
	std::mt19937 g(SEED + rand_cnt_for_placement);
	rand_cnt_for_placement++;

	int placement_num = 0;
	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		int ssd_index = NONE_ALLOC;
		vector<int> target_ssd_list;
		for (int ssd_temp = 0; ssd_temp < _num_of_SSDs; ssd_temp++) {
			if (!is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, ssd_temp, video_index) &&
				(_SSD_list[ssd_temp].bandwidth_usage + _VIDEO_SEGMENT_list[video_index].requested_bandwidth) <= _SSD_list[ssd_temp].maximum_bandwidth) {
				//&& ((_SSD_list[ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[video_index].size) / (_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD) / _SSD_list[ssd_temp].running_days) <= AVR_ADWD_LIMIT) {

				target_ssd_list.push_back(ssd_temp);
			}
		}
		std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);

		if (!target_ssd_list.empty()) {
			int ssd_index = target_ssd_list.back();
			allocate(_SSD_list, NULL, _VIDEO_SEGMENT_list, PLACEMENT_RANDOM, ssd_index, video_index);
			placement_num++;
		}
		/*else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_SEGMENT_list[video_index].is_alloc = false;
		}*/
	}
	return placement_num;
}
/*
int placement_random(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos) { // 밴드윗 생각 전혀 없음
	int placement_num = 0;
	std::mt19937 g(SEED + rand_cnt_for_placement);
	rand_cnt_for_placement++;
	vector<int> target_ssd_list;
	for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
		target_ssd_list.push_back(ssd);
	}
	std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);
	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		int ssd_index = NONE_ALLOC;
		while (!target_ssd_list.empty()) {
			ssd_index = target_ssd_list.back();
			if (is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, ssd_index, video_index))
				target_ssd_list.pop_back();
			else {
				allocate(_SSD_list, _VIDEO_SEGMENT_list, video_index, ssd_index);
				placement_num++;
				break;
			}
		}
	}
	return placement_num;
}
*/

void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _method, int _ssd_index, int _video_index) {
	bool is_removed = false;
	if (is_not_enough_storage_space(_SSD_list, _new_VIDEO_SEGMENT_list, _ssd_index, _video_index)) {
		is_removed = true;
	}
	double removed_bandwidth = 0;
	if (is_removed && _method == PLACEMENT_OURS) {
		int curr_vid = (*_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		_SSD_list[_ssd_index].bandwidth_usage += (_new_VIDEO_SEGMENT_list[_video_index].requested_bandwidth - _existed_VIDEO_SEGMENT_list[curr_vid].requested_bandwidth);
		_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.begin());
		_new_VIDEO_SEGMENT_list[curr_vid].assigned_SSD = NONE_ALLOC;
		_new_VIDEO_SEGMENT_list[curr_vid].is_alloc = false;
	}
	else {
		_SSD_list[_ssd_index].storage_usage += _new_VIDEO_SEGMENT_list[_video_index].size;
		_SSD_list[_ssd_index].bandwidth_usage += _new_VIDEO_SEGMENT_list[_video_index].requested_bandwidth;
	}

	_new_VIDEO_SEGMENT_list[_video_index].assigned_SSD = _ssd_index;
	_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_new_VIDEO_SEGMENT_list[_video_index].requested_bandwidth, _video_index));

	_new_VIDEO_SEGMENT_list[_video_index].is_alloc = true;

	//220120
	_SSD_list[_ssd_index].daily_write_MB += _new_VIDEO_SEGMENT_list[_video_index].size;
	_SSD_list[_ssd_index].ADWD += _new_VIDEO_SEGMENT_list[_video_index].size / (_SSD_list[_ssd_index].storage_capacity * _SSD_list[_ssd_index].DWPD);

	_SSD_list[_ssd_index].total_write_MB += _new_VIDEO_SEGMENT_list[_video_index].size;
}