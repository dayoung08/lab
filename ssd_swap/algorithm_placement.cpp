#include "header.h"

int placement(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _method, int _num_of_SSDs, int _num_of_videos) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
	case PLACEMENT_BANDWIDTH_AWARE:
	case PLACEMENT_STORAGE_SPACE_AWARE:
	case PLACEMENT_LIFETIME_AWARE:
		placement_num = placement_resource_aware(_SSD_list, _VIDEO_CHUNK_list, _method, _num_of_SSDs, _num_of_videos);
		break;
	case PLACEMENT_RANDOM:
	case PLACEMENT_ROUND_ROBIN:
		placement_num = placement_basic(_SSD_list, _VIDEO_CHUNK_list, _method, _num_of_SSDs, _num_of_videos);
		break;
	}

	//set_serviced_video(_SSD_list, _VIDEO_CHUNK_list, _num_of_SSDs, _num_of_videos);
	return placement_num;
}

int placement_resource_aware(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _placement_method, int _num_of_SSDs, int _num_of_videos) {
	set<pair<double, int>, greater<pair<double, int>>> VIDEO_CHUNK_list_with_bandwidth_sort;
	for (int vid = 0; vid < _num_of_videos; vid++) {
		VIDEO_CHUNK_list_with_bandwidth_sort.insert(make_pair(_VIDEO_CHUNK_list[vid].requested_bandwidth, vid));
	}

	int placement_num = 0;
	while (!VIDEO_CHUNK_list_with_bandwidth_sort.empty()) {
		int video_index = (*VIDEO_CHUNK_list_with_bandwidth_sort.begin()).second;
		VIDEO_CHUNK_list_with_bandwidth_sort.erase(*VIDEO_CHUNK_list_with_bandwidth_sort.begin());
		set<pair<double, int>, greater<pair<double, int>>> target_ssd_list_with_ratio_sort;

		for (int ssd_temp = 1; ssd_temp <= _num_of_SSDs; ssd_temp++) {
			if (!is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, ssd_temp, video_index)) {
				double remained_bandwidth = (_SSD_list[ssd_temp].maximum_bandwidth - _SSD_list[ssd_temp].total_bandwidth_usage);
				double remained_storage = (_SSD_list[ssd_temp].storage_capacity - _SSD_list[ssd_temp].storage_usage);

				double slope = -INFINITY;
				switch (_placement_method) {
				case PLACEMENT_OURS:
					if ((remained_storage - _VIDEO_CHUNK_list[video_index].size) == 0)
						slope = -INFINITY;
					else
						slope = (remained_bandwidth - _VIDEO_CHUNK_list[video_index].requested_bandwidth) / (remained_storage - _VIDEO_CHUNK_list[video_index].size);
					break;
				case PLACEMENT_BANDWIDTH_AWARE:
					slope = remained_bandwidth;
					break;
				case PLACEMENT_STORAGE_SPACE_AWARE:
					slope = remained_storage;
					break;
				case PLACEMENT_LIFETIME_AWARE:
					slope = 1/ _SSD_list[ssd_temp].ADWD; // 수명 많이 남은 게 너무 일찍 차버리면서 , 결국 수명 얼마 안 남은(DWPD 낮은) SSD에 할당을 더 많이 하게 되는 부작용 발생.
					break;
				}
				target_ssd_list_with_ratio_sort.insert(make_pair(slope, ssd_temp));
			}
		}

		if (!target_ssd_list_with_ratio_sort.empty()) {
			int ssd_index = (*target_ssd_list_with_ratio_sort.begin()).second;
			int prev_SSD = _VIDEO_CHUNK_list[video_index].assigned_SSD;
			double remained_bandwidth = (_SSD_list[ssd_index].maximum_bandwidth - (_SSD_list[ssd_index].total_bandwidth_usage + _VIDEO_CHUNK_list[video_index].requested_bandwidth));
			if (remained_bandwidth > 0) {
				allocate(_SSD_list, _VIDEO_CHUNK_list, ssd_index, video_index);
				if (prev_SSD != ssd_index) {
					placement_num++;
				}
			}
		}
		else {
			_VIDEO_CHUNK_list[video_index].assigned_SSD = NONE_ALLOC;
		}
	}
	return placement_num;
}


int placement_basic(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _placement_method, int _num_of_SSDs, int _num_of_videos) {
	default_random_engine g(SEED);

	int placement_num = 0;
 	vector<int> target_ssd_list;
	for (int ssd_temp = 1; ssd_temp <= _num_of_SSDs; ssd_temp++) {
		target_ssd_list.push_back(ssd_temp);
	}

	/*if (_placement_method == PLACEMENT_RANDOM)
		std::shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);*/
	//실수로 특허 때 이 위치 여기였었음. migration 전에 있는 배치는 애초에 어떻게 하든 문제가 되지 않아서 상관은 없는데 (비디오도 이미 대역폭이 랜덤셔플되어있어서 랜덤 할당한 것과 같은 상태)
	//그래도 사람이 불안하니까 이렇게 적어둔다.

	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		if (!target_ssd_list.empty()) {
			int ssd_index = NONE_ALLOC;
			if (_placement_method == PLACEMENT_RANDOM) {
				shuffle(target_ssd_list.begin(), target_ssd_list.end(), g);
				ssd_index = target_ssd_list.back();
			}
			if (_placement_method == PLACEMENT_ROUND_ROBIN) {
				ssd_index = target_ssd_list[vid % target_ssd_list.size()];
			}

			if (!is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, ssd_index, video_index)) {
				int prev_SSD = _VIDEO_CHUNK_list[video_index].assigned_SSD;
				double remained_bandwidth = (_SSD_list[ssd_index].maximum_bandwidth - (_SSD_list[ssd_index].total_bandwidth_usage + _VIDEO_CHUNK_list[video_index].requested_bandwidth));
				if (remained_bandwidth > 0) {
					allocate(_SSD_list, _VIDEO_CHUNK_list, ssd_index, video_index);
					if (prev_SSD != ssd_index) {
						placement_num++;
					}
				}
			}
			else {
				if (_placement_method == PLACEMENT_RANDOM)
					target_ssd_list.pop_back();
				if (_placement_method == PLACEMENT_ROUND_ROBIN)
					target_ssd_list.erase(target_ssd_list.begin() + (vid % target_ssd_list.size()));
			}
		}
		/*else {
			printf("video %d 를 저장할 만한 SSD가 없음\n", video_index);
			//_VIDEO_CHUNK_list[video_index].assigned_SSD = NONE_ALLOC;
			//_VIDEO_CHUNK_list[video_index].is_alloc = false;
		}*/
	}
	//cout << _VIDEO_CHUNK_list[1].assigned_SSD << endl;
	//cout << _VIDEO_CHUNK_list[10].assigned_SSD << endl;
	return placement_num;
}

void allocate(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _ssd_index, int _video_index) {
	int prev_SSD = _VIDEO_CHUNK_list[_video_index].assigned_SSD;
	_VIDEO_CHUNK_list[_video_index].assigned_SSD = _ssd_index;
	_SSD_list[_ssd_index].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_CHUNK_list[_video_index].requested_bandwidth, _video_index));
	_SSD_list[_ssd_index].storage_usage += _VIDEO_CHUNK_list[_video_index].size;
	_SSD_list[_ssd_index].total_bandwidth_usage += _VIDEO_CHUNK_list[_video_index].requested_bandwidth;

	if (prev_SSD != _ssd_index) {
		_SSD_list[_ssd_index].total_write_MB += _VIDEO_CHUNK_list[_video_index].size;
		_SSD_list[_ssd_index].ADWD = _SSD_list[_ssd_index].total_write_MB / (_SSD_list[_ssd_index].DWPD * _SSD_list[_ssd_index].storage_capacity * _SSD_list[_ssd_index].running_days);
	}
	//_VIDEO_CHUNK_list[_video_index].is_serviced = true;
	//_SSD_list[_ssd_index].serviced_bandwidth_usage += _VIDEO_CHUNK_list[_video_index].requested_bandwidth;
	/*if (_SSD_list[_ssd_index].total_bandwidth_usage > _SSD_list[_ssd_index].maximum_bandwidth) {
		_VIDEO_CHUNK_list[_video_index].is_serviced = false;
	}
	else {

	}*/
}