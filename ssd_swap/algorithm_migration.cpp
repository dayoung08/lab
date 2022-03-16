#include "header.h"
#define FLAG_REALLOCATE 0
#define FLAG_SWAP 1
#define FLAG_DENY -1

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	int migration_num = 0;
	switch (_migration_method) {
	case MIGRATION_OURS:
	case MIGRATION_BANDWIDTH_AWARE:
	case MIGRATION_STORAGE_SPACE_AWARE:
	case MIGRATION_LIFETIME_AWARE:
		migration_num = migration_resource_aware(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, _num_of_SSDs, _num_of_videos);
		break;
	}

	set_serviced_video(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_videos);
	return migration_num;
}

int migration_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	bool* is_over_load = new bool[_num_of_SSDs];
	bool* is_imposible = new bool[_num_of_SSDs];
	fill(is_imposible, is_imposible + _num_of_SSDs, false);

	set<pair<double, int>, greater<pair<double, int>>> over_load_SSDs; 
	vector<pair<double, int>> eliminated_video_list;
	bool is_inserted_eliminated_video_list = false;
	update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs, &eliminated_video_list, &is_inserted_eliminated_video_list);
	//printf("num_of_over_load : %d\n", over_load_SSDs.size());
	//여기까지 초기화

	int migration_num = 0;
	while (!over_load_SSDs.empty()) {
		int from_ssd = (*over_load_SSDs.begin()).second;
		set<pair<double, int>>::iterator end_pos = _SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.end();
		pair<double, int> element = (*--end_pos); //그 SSD에서 제일 큰 것을 먼저 빼서 다른 데로 옮길예정
		int from_vid = element.second;

		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int to_vid_temp = (*_SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				double slope_to = get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid);
				double slope_from = get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid);
				double ADWD = ((_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / _SSD_list[to_ssd_temp].running_days) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity);
				double remained_bandwidth = 0;
				double remained_storage = 0;

				if (is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, from_vid)) {
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth)));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				}
				else {
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth)));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - (_SSD_list[to_ssd_temp].storage_usage + _VIDEO_SEGMENT_list[from_vid].size);
				}

				double slope = -INFINITY;
				switch (_migration_method)
				{
				case MIGRATION_OURS:
					//slope = min(slope_to, slope_from);
					slope = (remained_bandwidth / remained_storage) / ADWD;
					break;
				case MIGRATION_BANDWIDTH_AWARE:
					slope = remained_bandwidth;
					break;
				case MIGRATION_STORAGE_SPACE_AWARE:
					slope = remained_storage;
					break;
				case MIGRATION_LIFETIME_AWARE:
					slope = 1-ADWD;
					break;
				}
				under_load_list.insert(make_pair(slope, to_ssd_temp));
			}
		}

		int to_vid = NONE_ALLOC;
		int to_ssd = NONE_ALLOC;
		int flag = FLAG_DENY;
		while (!under_load_list.empty()) {
			to_ssd = (*under_load_list.begin()).second;
			under_load_list.erase(*under_load_list.begin());

			if (to_ssd == from_ssd) {
				to_ssd = NONE_ALLOC;
				continue;
			}

			if (_SSD_list[to_ssd].total_assigned_VIDEOs_low_bandwidth_first.size()) { // 옮겼을때 할당 가능한 경우들
				to_vid = (*_SSD_list[to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, from_ssd, to_ssd, from_vid, to_vid);
			}
			else {
				flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, from_ssd, to_ssd, from_vid, NONE_ALLOC);
			}
			if (flag != FLAG_DENY) {
				break;
			}
			to_ssd = NONE_ALLOC;
			to_vid = NONE_ALLOC;
		}


		//스왑이 불가능한 상황일 경우 어떻게 할 것인가?를 생각할 차례가 왔음.
		if (under_load_list.empty() && flag == FLAG_DENY) {
			is_imposible[from_ssd] = true;
			if (from_ssd == VIRTUAL_SSD) {
				vector<pair<double, int>> curr_set(_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.size());
				copy(_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin(), _SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.end(), curr_set.begin());
				while (!curr_set.empty()) {
					int vid = curr_set.back().second;
					_VIDEO_SEGMENT_list[vid].assigned_SSD = NONE_ALLOC;
					_VIDEO_SEGMENT_list[vid].is_serviced = false;
					curr_set.pop_back();
				}
				curr_set.clear();
				vector<pair<double, int>>().swap(curr_set); //메모리 해제를 위해
			} // 사실 이미 저장된거 굳이 뺄 필요는 없다.... 그래서 굳이 안 뺌
			//정상적 서비스 여부 자체는 알아서 마지막에 계산 해주니까
			update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs, &eliminated_video_list, &is_inserted_eliminated_video_list);
			continue;
			/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
				printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_capacity, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_capacity));
			}
			exit(0);*/

			//여기를 어떻게 할 지가 고민이야ㅠㅠ
		}

		//찾았으면 할당하기.
		//int flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, MIGRATION_BANDWIDTH_AWARE, from_ssd, to_ssd, from_vid, to_vid);
		switch (flag) {
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		}

		update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs, &eliminated_video_list, &is_inserted_eliminated_video_list);
		migration_num++;
		under_load_list.clear();
		set<pair<double, int>, greater<pair<double, int>>>().swap(under_load_list); //메모리 해제를 위해
	}

	/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	delete[] is_over_load;

	over_load_SSDs.clear();
	eliminated_video_list.clear();
	set<pair<double, int>, greater<pair<double, int>>>().swap(over_load_SSDs);
	vector<pair<double, int>>().swap(eliminated_video_list);
	return migration_num;
}

void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	_VIDEO_SEGMENT_list[_from_vid].is_serviced = true;
	_SSD_list[_to_ssd].total_bandwidth_usage += (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);

	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_to_vid].requested_bandwidth, _to_vid));
	_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = _from_ssd;
	_VIDEO_SEGMENT_list[_to_vid].is_serviced = true;
	_SSD_list[_from_ssd].total_bandwidth_usage -= (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);

	/*_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[from_ssd].storage_usage += _VIDEO_SEGMENT_list[to_vid].size;
	_SSD_list[to_ssd].storage_usage += _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].storage_usage -= _VIDEO_SEGMENT_list[to_vid].size;*/

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = (_SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity)) / _SSD_list[_to_ssd].running_days;
	if (_from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB += _VIDEO_SEGMENT_list[_to_vid].size;
		_SSD_list[_from_ssd].ADWD = (_SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity)) / _SSD_list[_from_ssd].running_days;
	}
}

void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> element, int from_ssd, int to_ssd, int from_vid) {
	_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(element);
	_SSD_list[to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[from_vid].requested_bandwidth, from_vid));
	_VIDEO_SEGMENT_list[from_vid].assigned_SSD = to_ssd;
	_VIDEO_SEGMENT_list[from_vid].is_serviced = true;

	_SSD_list[from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
	_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].total_bandwidth_usage += _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
	_SSD_list[to_ssd].storage_usage += _VIDEO_SEGMENT_list[from_vid].size;

	_SSD_list[to_ssd].total_write_MB += _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].ADWD = (_SSD_list[to_ssd].total_write_MB / (_SSD_list[to_ssd].DWPD * _SSD_list[to_ssd].storage_capacity)) / _SSD_list[to_ssd].running_days;
}

void update_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, bool* _is_over_load, bool* _is_imposible, set<pair<double, int>, greater<pair<double, int>>>* _over_load_SSDs, int _num_of_SSDs, vector<pair<double, int>>* _eliminated_video_list, bool* _is_inserted_eliminated_video_list) {
	int _num_of_over_load = 0;
	(*_over_load_SSDs).clear();
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_is_imposible[ssd])
			continue;
		if (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			if (ssd == VIRTUAL_SSD && _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.empty()) {
				_is_over_load[ssd] = false;
				continue;
			}
			_is_over_load[ssd] = true;
			if (_migration_method == MIGRATION_OURS) { 
				double throughput = (_SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth) / (_SSD_list[ssd].storage_capacity - _SSD_list[ssd].storage_usage);
				(*_over_load_SSDs).insert(make_pair(throughput, ssd));
			}
			else
				(*_over_load_SSDs).insert(make_pair(_SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
			_num_of_over_load++;
		}
		else
			_is_over_load[ssd] = false;
	}

	if (_num_of_over_load == 0 && _migration_method == MIGRATION_OURS && (*_eliminated_video_list).size()) {
		while ((*_eliminated_video_list).size()) {
			_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert((*_eliminated_video_list).back());
			int curr_vid = (*_eliminated_video_list).back().second;
			(*_eliminated_video_list).pop_back();
			_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _VIDEO_SEGMENT_list[curr_vid].requested_bandwidth;
			_SSD_list[VIRTUAL_SSD].storage_usage += _VIDEO_SEGMENT_list[curr_vid].size;
		}
		*_is_inserted_eliminated_video_list = true;
		_num_of_over_load = 1;
		double throughput = (_SSD_list[VIRTUAL_SSD].total_bandwidth_usage - _SSD_list[VIRTUAL_SSD].maximum_bandwidth) / (_SSD_list[VIRTUAL_SSD].storage_capacity - _SSD_list[VIRTUAL_SSD].storage_usage);
		(*_over_load_SSDs).insert(make_pair(throughput, VIRTUAL_SSD));
		//(*_over_load_SSDs).insert(make_pair(_SSD_list[VIRTUAL_SSD].total_bandwidth_usage - _SSD_list[VIRTUAL_SSD].maximum_bandwidth, VIRTUAL_SSD));
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}


double get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double difference_bt;
	double slope_to;
	double ADWD_to;
	double remained_bandwidth;
	double remained_storage;
	//double remained_write_MB;

	if (is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
		ADWD_to = ((_SSD_list[_to_ssd].total_write_MB + _VIDEO_SEGMENT_list[_from_vid].size) / _SSD_list[_to_ssd].running_days) / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		remained_bandwidth = (_SSD_list[_to_ssd].maximum_bandwidth - (_SSD_list[_to_ssd].total_bandwidth_usage + (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth)));
		remained_storage = _SSD_list[_to_ssd].storage_capacity - _SSD_list[_to_ssd].storage_usage;
	}
	else {
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth);
		ADWD_to = ((_SSD_list[_to_ssd].total_write_MB + _VIDEO_SEGMENT_list[_from_vid].size) / _SSD_list[_to_ssd].running_days) / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		remained_bandwidth = (_SSD_list[_to_ssd].maximum_bandwidth - (_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth));
		remained_storage = _SSD_list[_to_ssd].storage_capacity - (_SSD_list[_to_ssd].storage_usage + _VIDEO_SEGMENT_list[_from_vid].size);
	}
	slope_to =  (remained_bandwidth / remained_storage) / ADWD_to;
	//slope_to = difference_bt / ADWD_to;
	return slope_to;
}

double get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double difference_bt;
	double slope_from;
	double ADWD_from;
	double remained_bandwidth;
	double remained_storage;

	if (is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth); 
		ADWD_from = ((_SSD_list[_from_ssd].total_write_MB + _VIDEO_SEGMENT_list[_to_vid].size) / _SSD_list[_from_ssd].running_days) / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity);
		remained_bandwidth = (_SSD_list[_from_ssd].maximum_bandwidth - (_SSD_list[_from_ssd].total_bandwidth_usage - (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth)));
		remained_storage = _SSD_list[_from_ssd].storage_capacity - _SSD_list[_from_ssd].storage_usage;
	}
	else {
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth);
		ADWD_from = (_SSD_list[_from_ssd].total_write_MB / _SSD_list[_from_ssd].running_days) / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity);
		remained_bandwidth = (_SSD_list[_from_ssd].maximum_bandwidth - (_SSD_list[_from_ssd].total_bandwidth_usage - _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth));
		remained_storage = _SSD_list[_from_ssd].storage_capacity - (_SSD_list[_from_ssd].storage_usage - _VIDEO_SEGMENT_list[_from_vid].size);
	}
	slope_from = (remained_bandwidth / remained_storage) / ADWD_from;
	//slope_from = difference_bt / ADWD_from;

	return slope_from;
}

int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	int flag = FLAG_DENY;

	if (_to_vid != NONE_ALLOC) {
		if (!is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
			if(_from_ssd == VIRTUAL_SSD)
				flag = FLAG_REALLOCATE;
			else if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth) {
				flag = FLAG_REALLOCATE;
			}
		}
		else if (_to_ssd != NONE_ALLOC) {
			if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth &&
				_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth > _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) {
				flag = FLAG_SWAP;
			}
		}
	}
	else { // 여기로 오는 경우는 _to_ssd가 빈 경우
		//if (!is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) // 사실 이것도 필요없을 것...
		flag = FLAG_REALLOCATE;
	}
	//만약 AVR_ADWD_LIMIT를 지정할 경우, flag를 바꾸기 위해 사용되는 IF문이 있었는데 제대로 동작 안하고, Limit 지정도 안하기로 해서 삭제함
	return flag;
}