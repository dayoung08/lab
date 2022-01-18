#include "header.h"
#define FLAG_REALLOCATE 0
#define FLAG_SWAP 1
#define FLAG_ELIMINATE -1

int migration(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _method) {
	int migration_num = 0;
	switch (_method) {
	case MIGRATION_OURS:
		migration_num = migration_myAlgorithm(_SSD_list, _VIDEO_list);
		break;
	case MIGRATION_BANDWIDTH_AWARE:
		migration_num = migration_bandwidth_aware(_SSD_list, _VIDEO_list);
		break;
	}
	return migration_num;
}

int migration_myAlgorithm(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	bool is_over_load[NUM_OF_SSDs + 1];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
	//printf("num_of_over_load : %d\n", bandwidth_usage_of_SSDs.size()); //여기까지 초기화

	int migration_num = 0;
	while (!bandwidth_usage_of_SSDs.empty()) {
		int from_ssd = (*bandwidth_usage_of_SSDs.begin()).second;
		set<pair<double, int>>::iterator pos = _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.end(); 
		pair<double, int> element = (*--pos); //그 SSD에서 제일 큰 것을 먼저 빼서 다른 데로 옮길예정
		int from_vid = element.second;
		_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);

		//할당 가능한 ssd 찾기, swap할 VIDEO도 찾기.
		set<pair<double, int>, less<pair<double, int>>> ADWD_after_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= NUM_OF_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int flag_temp = get_swap_flag(_SSD_list, _VIDEO_list, from_vid, to_ssd_temp);
				if (flag_temp != FLAG_ELIMINATE) {
					double slope_to = (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd_temp, from_vid, flag_temp)).second;
					double slope_from = (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd_temp, from_vid, flag_temp)).second;
					double slope = max(slope_to, slope_from);
					ADWD_after_list.insert(make_pair(slope, to_ssd_temp));
				}
			}
		}

		// SSD 선택 / 만약 swap할 경우, swap할 비디오 선택
		int to_ssd, to_vid, flag;
		while (!ADWD_after_list.empty()) {
			to_ssd = (*ADWD_after_list.begin()).second;
			ADWD_after_list.erase(*ADWD_after_list.begin());
			if (to_ssd == from_ssd)
				continue;

			flag = get_swap_flag(_SSD_list, _VIDEO_list, from_vid, to_ssd);
			if (flag == FLAG_REALLOCATE) {
				break;
			}
			else if (flag == FLAG_SWAP) {
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				break;
			}
		}

		//위의 결과에 따라 처리하기
		if (flag == FLAG_ELIMINATE) {
			eliminate(_SSD_list, _VIDEO_list, from_ssd, from_vid);
		}
		else {
			if (flag == FLAG_REALLOCATE) {
				reallocate(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid);
			}
			else if (flag == FLAG_SWAP) {
				swap(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, to_vid);
			}
			_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, flag)).first;
			_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, flag)).first;
			migration_num++;
		}
		
		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
	}

	/*for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	return migration_num;
}


int migration_bandwidth_aware(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	bool is_over_load[NUM_OF_SSDs + 1];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
	//printf("num_of_over_load : %d\n", bandwidth_usage_of_SSDs.size()); //여기까지 초기화

	int migration_num = 0;
	while (!bandwidth_usage_of_SSDs.empty()) {
		int from_ssd = (*bandwidth_usage_of_SSDs.begin()).second;
		set<pair<double, int>>::iterator pos = _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.end();
		pair<double, int> element = (*--pos); //그 SSD에서 밴드윗이 제일 큰 것을 먼저 빼서 다른 데로 옮길예정

		int from_vid = element.second;

		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= NUM_OF_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int flag_temp = get_swap_flag(_SSD_list, _VIDEO_list, from_vid, to_ssd_temp);
				if (flag_temp != FLAG_ELIMINATE) {
					double remaining_bt;
					int to_vid_temp = (*_SSD_list[to_ssd_temp].assigned_VIDEOs_low_bandwidth_first.begin()).second;
					if (flag_temp == FLAG_REALLOCATE) {
						remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - _VIDEO_list[from_vid].requested_bandwidth;
					}
					else if (flag_temp == FLAG_SWAP) {
						remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid_temp].requested_bandwidth);
					}
					under_load_list.insert(make_pair(remaining_bt, to_ssd_temp));
				}
			}
		}

		// SSD 선택 / 만약 swap할 경우, swap할 비디오 선택
		int to_ssd, to_vid, flag;
		while (!under_load_list.empty()) {
			to_ssd = (*under_load_list.begin()).second;
			under_load_list.erase(*under_load_list.begin());
			if (to_ssd == from_ssd)
				continue;

			flag = get_swap_flag(_SSD_list, _VIDEO_list, from_vid, to_ssd);
			if (flag == FLAG_REALLOCATE) {
				break;
			}
			else if (flag == FLAG_SWAP) {
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				break;
			}
		}

		//위의 결과에 따라 처리하기
		if (flag == FLAG_ELIMINATE) {
			eliminate(_SSD_list, _VIDEO_list, from_ssd, from_vid);
		}
		else {
			if (flag == FLAG_REALLOCATE) {
				reallocate(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid);
			}
			else if (flag == FLAG_SWAP) {
				swap(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, to_vid);
			}
			_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, flag)).first;
			_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid, flag)).first;
			migration_num++;
		}

		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
	}

	/*for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	return migration_num;
}


int get_swap_flag(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int from_vid, int to_ssd) {
	if (_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.empty()) {
		// 그냥 옮기면 됨. swap 아님.
		return 0;
	}
	else {
		int to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		int from_ssd = _VIDEO_list[from_vid].assigned_SSD;
		if (((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) <= _SSD_list[to_ssd].storage_space) &&
			((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth) <= _SSD_list[to_ssd].maximum_bandwidth)) {
			// 그냥 옮기면 됨. swap 아님.
			return FLAG_REALLOCATE;
		}
		else if ((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth) <= _SSD_list[to_ssd].maximum_bandwidth &&
			(_SSD_list[from_ssd].bandwidth_usage + _VIDEO_list[to_vid].requested_bandwidth - _VIDEO_list[from_vid].requested_bandwidth) <= _SSD_list[from_ssd].maximum_bandwidth) {
			// swap 해야할 경우
			return FLAG_SWAP;
		}
		else {
			//할당 불가능
			return FLAG_ELIMINATE;
		}
	}
}

void swap(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	//_from_vid는 이미 _from_ssd에서 삭제함
	_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()); //_to_vid를 _to_ssd에서 삭제

	_SSD_list[_from_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_to_vid].requested_bandwidth, _to_vid));
	_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_from_vid].requested_bandwidth, _from_vid));

	_VIDEO_list[_from_vid].assigned_SSD = _to_ssd;
	_VIDEO_list[_to_vid].assigned_SSD = _from_ssd;
	_SSD_list[_from_ssd].bandwidth_usage -= (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[_to_vid].requested_bandwidth);
	_SSD_list[_to_ssd].bandwidth_usage += (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[_to_vid].requested_bandwidth);
}

void reallocate(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid) {
	//_from_vid는 이미 _from_ssd에서 삭제함
	_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_list[_from_vid].assigned_SSD = _to_ssd;

	_SSD_list[_from_ssd].bandwidth_usage -= _VIDEO_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= SIZE_OF_VIDEO;
	_SSD_list[_to_ssd].bandwidth_usage += _VIDEO_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += SIZE_OF_VIDEO;
}

void eliminate(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _from_vid) {
	//_from_vid는 이미 _from_ssd에서 삭제함
	_VIDEO_list[_from_vid].assigned_SSD = 0;
	_VIDEO_list[_from_vid].is_alloc = false;

	_SSD_list[_from_ssd].bandwidth_usage -= _VIDEO_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= SIZE_OF_VIDEO;
}

void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* bandwidth_usage_of_SSDs) {
	int _num_of_over_load = 0;
	(*bandwidth_usage_of_SSDs).clear();
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		if (_SSD_list[ssd].bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			_is_over_load[ssd] = true;
			(*bandwidth_usage_of_SSDs).insert(make_pair(_SSD_list[ssd].bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
			_num_of_over_load++;
		}
		else
			_is_over_load[ssd] = false;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}

//get_slope_to, get_slope_from 둘 다 flag == FLAG_ELIMINATE이면 진입하면 안됨.
pair<double, double> get_slope_to(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid, int flag) {
	double ADWD_to = SIZE_OF_VIDEO / (_SSD_list[_to_ssd].storage_space * _SSD_list[_to_ssd].DWPD);
	double bt_difference;
	if (flag == FLAG_REALLOCATE) {
		bt_difference = _VIDEO_list[_from_vid].requested_bandwidth;
	}
	else if (flag == FLAG_SWAP) {
		int to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);
	}
	double slope_to =  (_SSD_list[_to_ssd].ADWD + ADWD_to) / bt_difference;

	return make_pair(ADWD_to, slope_to);
}

pair<double, double> get_slope_from(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid, int flag) {
	double ADWD_from, bt_difference;
	if (flag == FLAG_REALLOCATE) {
		bt_difference = _VIDEO_list[_from_vid].requested_bandwidth;
		ADWD_from = 0;
	}
	else if (flag == FLAG_SWAP) {
		int to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);
		ADWD_from = SIZE_OF_VIDEO / (_SSD_list[_to_ssd].storage_space * _SSD_list[_to_ssd].DWPD);
	}
	double slope_from = (_SSD_list[_from_ssd].ADWD + ADWD_from) / bt_difference;

	return make_pair(ADWD_from, slope_from);
}
