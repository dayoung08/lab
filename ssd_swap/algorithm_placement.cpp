#include "header.h"

int rand_cnt_for_placement = 0; //for generation seed in placement_random
int placement(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _num_of_SSDs, int _num_of_videos) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
	case PLACEMENT_BANDWIDTH_AWARE:
	case PLACEMENT_STORAGE_SPACE_AWARE:
	case PLACEMENT_LIFETIME_AWARE:
		placement_num = placement_resource_aware(_SSD_list, _VIDEO_SEGMENT_list, _method, _num_of_SSDs, _num_of_videos);
		break;
	case PLACEMENT_RANDOM:
	case PLACEMENT_ROUND_ROBIN:
		placement_num = placement_basic(_SSD_list, _VIDEO_SEGMENT_list, _method, _num_of_SSDs, _num_of_videos);
		break;
	}

	set_serviced_video(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_videos);
	return placement_num;
}

int placement_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _placement_method, int _num_of_SSDs, int _num_of_videos) {
	set<pair<double, int>, greater<pair<double, int>>> VIDEO_SEGMENT_list_with_bandwidth_sort;
	for (int vid = 0; vid < _num_of_videos; vid++) {
		VIDEO_SEGMENT_list_with_bandwidth_sort.insert(make_pair(_VIDEO_SEGMENT_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!VIDEO_SEGMENT_list_with_bandwidth_sort.empty()) {
		int video_index = (*VIDEO_SEGMENT_list_with_bandwidth_sort.begin()).second;
		VIDEO_SEGMENT_list_with_bandwidth_sort.erase(*VIDEO_SEGMENT_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_ratio_sort;

		for (int ssd_temp = 1; ssd_temp <= _num_of_SSDs; ssd_temp++) {
			if (!is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, ssd_temp, video_index)) {
				double remained_bandwidth = (_SSD_list[ssd_temp].maximum_bandwidth - (_SSD_list[ssd_temp].total_bandwidth_usage + _VIDEO_SEGMENT_list[video_index].requested_bandwidth));
				double remained_storage = (_SSD_list[ssd_temp].storage_capacity - (_SSD_list[ssd_temp].storage_usage + _VIDEO_SEGMENT_list[video_index].size));
				//double remained_write_MB = ((_SSD_list[ssd_temp].storage_capacity * _SSD_list[ssd_temp].DWPD) * _SSD_list[ssd_temp].running_days) - (_SSD_list[ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[video_index].size);
				double ADWD = ((_SSD_list[ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[video_index].size) / _SSD_list[ssd_temp].running_days) / (_SSD_list[ssd_temp].DWPD * _SSD_list[ssd_temp].storage_capacity);

				double slope = -INFINITY;
				switch (_placement_method) {
				case PLACEMENT_OURS:
					slope = remained_bandwidth / remained_storage;
					break;
				case PLACEMENT_BANDWIDTH_AWARE:
					slope = remained_bandwidth;
					break;
				case PLACEMENT_STORAGE_SPACE_AWARE:
					slope = remained_storage;
					break;
				case PLACEMENT_LIFETIME_AWARE:
					slope = 1-ADWD; // 수명 많이 남은 게 너무 일찍 차버리면서 , 결국 수명 얼마 안 남은(DWPD 낮은) SSD에 할당을 더 많이 하게 되는 부작용 발생.
					break;
				}
				target_ssd_list_with_ratio_sort.insert(make_pair(slope, ssd_temp));
			}
		}

		if (!target_ssd_list_with_ratio_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_ratio_sort.begin()).second;
			int prev_SSD = _VIDEO_SEGMENT_list[video_index].assigned_SSD;
			if (prev_SSD != ssd_index) {
				placement_num++;
			}

			allocate(_SSD_list, _VIDEO_SEGMENT_list, ssd_index, video_index);
		}
		else {
			/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
				printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] storage %.2f  / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_capacity, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_capacity));
				printf("[SSD %d] Average ADWD %.2f \n", ssd, (_SSD_list[ssd].total_write_MB) / (_SSD_list[ssd].storage_capacity * _SSD_list[ssd].DWPD) / _SSD_list[ssd].running_days);
			}*/
			_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
			_VIDEO_SEGMENT_list[video_index].is_serviced = false;
		}
		/*else {
			for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
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


int placement_basic(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _placement_method, int _num_of_SSDs, int _num_of_videos) {
	std::mt19937 g(SEED + rand_cnt_for_placement);
	rand_cnt_for_placement++;

	int placement_num = 0;
	vector<int> target_ssd_list;
	for (int ssd_temp = 1; ssd_temp <= _num_of_SSDs; ssd_temp++) {
		target_ssd_list.push_back(ssd_temp);
	}

	/*if (_placement_method == PLACEMENT_RANDOM)
		std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);*/
	//실수로 migration 전에 배치때 이 위치 여기였었음. 액셀 쪽 이렇게 되어있으니까 나중에 생각 꼭 해놓자.
	//migration 전에 있는 배치는 애초에 어떻게 하든 문제가 되지 않아서 상관은 없는데 (비디오도 이미 대역폭이 랜덤셔플되어있어서 랜덤 할당한 것과 같은 상태)
	//그래도 사람이 불안하니까 이렇게 적어둔다.

	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		if (_placement_method == PLACEMENT_RANDOM)
			std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);

		if (!target_ssd_list.empty()) {
			int ssd_index = target_ssd_list.back();
			if (_placement_method == PLACEMENT_ROUND_ROBIN)
				ssd_index %= _num_of_videos;

			if (!is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, ssd_index, video_index)) {
				int prev_SSD = _VIDEO_SEGMENT_list[video_index].assigned_SSD;
				if (prev_SSD != ssd_index) {
					placement_num++;
				}

				allocate(_SSD_list, _VIDEO_SEGMENT_list, ssd_index, video_index);
			}
			else {
				target_ssd_list.pop_back();
			}
		}
		/*else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_SEGMENT_list[video_index].is_alloc = false;
		}*/
	}
	return placement_num;
}

void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _ssd_index, int _video_index) {
	int prev_SSD = _VIDEO_SEGMENT_list[_video_index].assigned_SSD;
	_VIDEO_SEGMENT_list[_video_index].assigned_SSD = _ssd_index;
	_SSD_list[_ssd_index].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_video_index].requested_bandwidth, _video_index));
	_SSD_list[_ssd_index].storage_usage += _VIDEO_SEGMENT_list[_video_index].size;
	_SSD_list[_ssd_index].total_bandwidth_usage += _VIDEO_SEGMENT_list[_video_index].requested_bandwidth;

	if (prev_SSD != _ssd_index) {
		_SSD_list[_ssd_index].total_write_MB += _VIDEO_SEGMENT_list[_video_index].size;
		_SSD_list[_ssd_index].ADWD = (_SSD_list[_ssd_index].total_write_MB / _SSD_list[_ssd_index].running_days) / (_SSD_list[_ssd_index].DWPD * _SSD_list[_ssd_index].storage_capacity);
	}
}