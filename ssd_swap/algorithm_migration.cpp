#include "header.h"
#define FLAG_REALLOCATE 0
#define FLAG_SWAP 1
#define FLAG_DENY -1

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _num_of_SSDs) {
	int migration_num = 0;
	switch (_method) {
	case MIGRATION_OURS:
		migration_num = migration_myAlgorithm(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs);
		break;
	case MIGRATION_BANDWIDTH_AWARE:
		migration_num = migration_bandwidth_aware(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs);
		break;
	}

	/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	return migration_num;
}


int migration_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs) {
	bool* is_over_load = new bool[_num_of_SSDs];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
	//printf("num_of_over_load : %d\n", bandwidth_usage_of_SSDs.size());
	//여기까지 초기화

	int migration_num = 0;
	while (!bandwidth_usage_of_SSDs.empty()) {
		int from_ssd = (*bandwidth_usage_of_SSDs.begin()).second;
		set<pair<double, int>>::iterator pos = _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.end();
		pair<double, int> element = (*--pos); //그 SSD에서 제일 큰 것을 먼저 빼서 다른 데로 옮길예정

		int from_vid = element.second;

		//sort 하기
		set<pair<double, int>, less<pair<double, int>>> ADWD_after_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				double slope_to = (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid)).second;
				double slope_from = (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid)).second;
				double slope = max(slope_to, slope_from);
				ADWD_after_list.insert(make_pair(slope, to_ssd_temp));
			}
		}

		//할당 가능한 ssd 찾기, swap할 VIDEO도 찾기.
		int to_vid = NONE_ALLOC;
		int to_ssd = NONE_ALLOC;
		while (!ADWD_after_list.empty()) {
			to_ssd = (*ADWD_after_list.begin()).second;
			ADWD_after_list.erase(*ADWD_after_list.begin());
			if (to_ssd == from_ssd) {
				to_ssd = NONE_ALLOC;
				continue;
			}

			if (!_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.empty())
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
			else
				to_vid = NONE_ALLOC;
			
			if (get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, MIGRATION_OURS, from_ssd, to_ssd, from_vid, to_vid) != FLAG_DENY) {
				break;
			}
		}

		//스왑이 불가능한 상황일 경우 어떻게 할 것인가?를 생각할 차례가 왔음.
		if (ADWD_after_list.empty()) {
			while (_SSD_list[from_ssd].bandwidth_usage > _SSD_list[from_ssd].maximum_bandwidth) {
				if (from_ssd == VIRTUAL_SSD && _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.empty()) {
					break;
				}
				int curr_vid = (*_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_SEGMENT_list[curr_vid].requested_bandwidth;
				_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[curr_vid].size;
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.begin());

				_VIDEO_SEGMENT_list[curr_vid].assigned_SSD = NONE_ALLOC;
			}
			update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
			continue;
			/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
				printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_capacity, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_capacity));
			}
			exit(0);*/

			//여기를 어떻게 할 지가 고민이야ㅠㅠ
		}

		//찾았으면 할당하기.
		int flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, MIGRATION_OURS, from_ssd, to_ssd, from_vid, to_vid);
		switch (flag) {
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		}

		_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.first;
		_SSD_list[to_ssd].daily_write_MB += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
		_SSD_list[to_ssd].total_write_MB += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;

		if (from_ssd != VIRTUAL_SSD) {
			_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.first;
			_SSD_list[from_ssd].total_write_MB += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
			_SSD_list[from_ssd].daily_write_MB += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
		}

		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
		migration_num++;
	}
	delete[] is_over_load;
	return migration_num;
}


int migration_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs) {
	bool* is_over_load = new bool[_num_of_SSDs];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
	//printf("num_of_over_load : %d\n", bandwidth_usage_of_SSDs.size());
	//여기까지 초기화

	int migration_num = 0;
	while (!bandwidth_usage_of_SSDs.empty()) {
		int from_ssd = (*bandwidth_usage_of_SSDs.begin()).second;
		set<pair<double, int>>::iterator pos = _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.end();
		pair<double, int> element = (*--pos); //그 SSD에서 제일 큰 것을 먼저 빼서 다른 데로 옮길예정

		int from_vid = element.second;

		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				double remaining_bt;
				int to_vid_temp = (*_SSD_list[to_ssd_temp].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				if (is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, from_vid)) {
					remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth);
				}
				else {
					remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
				}
				under_load_list.insert(make_pair(remaining_bt, to_ssd_temp));
			}
		}

		int to_vid = NONE_ALLOC;
		int to_ssd = NONE_ALLOC;
		while (!under_load_list.empty()) {
			to_ssd = (*under_load_list.begin()).second;
			under_load_list.erase(*under_load_list.begin());
			if (to_ssd == from_ssd) {
				to_ssd = NONE_ALLOC;
				continue;
			}

			if (_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.size()) { // 옮겼을때 할당 가능한 경우들
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				if ((((_SSD_list[to_ssd].storage_usage + _VIDEO_SEGMENT_list[from_vid].size) > _SSD_list[to_ssd].storage_capacity)) && ((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) && (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth > _VIDEO_SEGMENT_list[to_vid].requested_bandwidth)) {
					break;
				}
				else if ((((_SSD_list[to_ssd].storage_usage + _VIDEO_SEGMENT_list[from_vid].size) <= _SSD_list[to_ssd].storage_capacity)) && (_SSD_list[to_ssd].bandwidth_usage + _VIDEO_SEGMENT_list[from_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) {
					break;
				}
			}
			else { // 마이그레이션 할 ssd가 비었을 경우
				break;
			}
		}

		//스왑이 불가능한 상황일 경우 어떻게 할 것인가?를 생각할 차례가 왔음.
		if (under_load_list.empty()) {
			while (_SSD_list[from_ssd].bandwidth_usage > _SSD_list[from_ssd].maximum_bandwidth) {
				if (from_ssd == VIRTUAL_SSD && _SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.empty()) {
					break;
				}
				int curr_vid = (*_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_SEGMENT_list[curr_vid].requested_bandwidth;
				_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[curr_vid].size;
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.begin());
				_VIDEO_SEGMENT_list[curr_vid].assigned_SSD = NONE_ALLOC;
			}
			update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
			continue;
			/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
				printf("[SSD %d] %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] %d / %d (%.2f%%)\n\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_capacity, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_capacity));
			}
			exit(0);*/
			//여기를 어떻게 할 지가 고민이야ㅠㅠ
		}

		//찾았으면 할당하기.
		int flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, MIGRATION_BANDWIDTH_AWARE, from_ssd, to_ssd, from_vid, to_vid);
		switch (flag) {
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		}

		_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.first;
		_SSD_list[to_ssd].daily_write_MB += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
		_SSD_list[to_ssd].total_write_MB += (get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;

		if (from_ssd != VIRTUAL_SSD) {
			_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.first;
			_SSD_list[from_ssd].total_write_MB += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
			_SSD_list[from_ssd].daily_write_MB += (get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd, from_vid)).first.second;
		}
		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs, _num_of_SSDs);
		migration_num++;
	}

	/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	delete[] is_over_load;
	return migration_num;
}

void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	_SSD_list[_from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin());
	
	_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	_SSD_list[_to_ssd].bandwidth_usage += (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);

	_SSD_list[_from_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_to_vid].requested_bandwidth, _to_vid));
	_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = _from_ssd;
	_SSD_list[_from_ssd].bandwidth_usage -= (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
	
	/*_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[from_ssd].storage_usage += _VIDEO_SEGMENT_list[to_vid].size;
	_SSD_list[to_ssd].storage_usage += _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].storage_usage -= _VIDEO_SEGMENT_list[to_vid].size;*/
}

void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> element, int from_ssd, int to_ssd, int from_vid) {
	_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
	_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[from_vid].requested_bandwidth, from_vid));
	_VIDEO_SEGMENT_list[from_vid].assigned_SSD = to_ssd;

	_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
	_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].bandwidth_usage += _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
	_SSD_list[to_ssd].storage_usage += _VIDEO_SEGMENT_list[from_vid].size;
}

void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs, int _num_of_SSDs) {
	int _num_of_over_load = 0;
	(*_bandwidth_usage_of_SSDs).clear();
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			if (ssd == VIRTUAL_SSD && _SSD_list[ssd].assigned_VIDEOs_low_bandwidth_first.empty()) {
				_is_over_load[ssd] = false;
				continue;
			}

			_is_over_load[ssd] = true;
			(*_bandwidth_usage_of_SSDs).insert(make_pair(_SSD_list[ssd].bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
			_num_of_over_load++;
		}
		else
			_is_over_load[ssd] = false;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}

pair<pair<double, double>, double> get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double ADWD_to;
	double slope_to;
	double bt_difference;
	//double difference_of_write_MB_to;
	double write_MB_to;

	if (is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
		ADWD_to = _VIDEO_SEGMENT_list[_from_vid].size / (_SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].DWPD);
		//difference_of_write_MB_to = (_SSD_list[_to_ssd].write_MB + _VIDEO_SEGMENT_list[_from_vid].size) - (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		write_MB_to = _VIDEO_SEGMENT_list[_from_vid].size;
	}
	else {
		bt_difference = _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
		ADWD_to = _VIDEO_SEGMENT_list[_from_vid].size / (_SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].DWPD);
		//difference_of_write_MB_to = (_SSD_list[_to_ssd].write_MB + _VIDEO_SEGMENT_list[_from_vid].size) - (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		write_MB_to = _VIDEO_SEGMENT_list[_from_vid].size;
	}
	slope_to = (_SSD_list[_to_ssd].ADWD + ADWD_to) / bt_difference;
	//slope_to = difference_of_write_MB_to / bt_difference;

	return make_pair(make_pair(ADWD_to, write_MB_to), slope_to);
}

pair<pair<double, double>, double> get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double ADWD_from;
	double slope_from;
	double bt_difference;
	//double difference_of_write_MB_from;
	double write_MB_from;

	if (is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
		ADWD_from = _VIDEO_SEGMENT_list[_to_vid].size / (_SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].DWPD);
		//difference_of_write_MB_from = (_SSD_list[_to_ssd].write_MB + _VIDEO_SEGMENT_list[_to_vid].size) - (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		write_MB_from = _VIDEO_SEGMENT_list[_to_vid].size;
	}
	else {
		bt_difference = _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
		ADWD_from = 0;
		//difference_of_write_MB_from = 0 - (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
		write_MB_from = 0;
	}
	slope_from = (_SSD_list[_from_ssd].ADWD + ADWD_from) / bt_difference;
	//slope_from = (difference_of_write_MB_from) / bt_difference;

	return make_pair(make_pair(ADWD_from, write_MB_from), slope_from);
}

int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	int flag = FLAG_DENY;

	if (_to_vid != NONE_ALLOC && _to_ssd != NONE_ALLOC) {
		if (!is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid) &&
			(_SSD_list[_to_ssd].bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth) < _SSD_list[_to_ssd].maximum_bandwidth) {
			flag = FLAG_REALLOCATE;
		}
		else if ( is_not_enough_storage_space(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid) &&
			(_SSD_list[_to_ssd].bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) < _SSD_list[_to_ssd].maximum_bandwidth &&
			_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth > _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) {
			flag = FLAG_SWAP;
		}
		/*else {
			flag = FLAG_DENY;
		}*/
	}
	else {
		flag = FLAG_REALLOCATE;
	}


	if (flag == FLAG_REALLOCATE && 
		((_SSD_list[_to_ssd].total_write_MB + _VIDEO_SEGMENT_list[_to_vid].size) / (_SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].DWPD) / _SSD_list[_to_ssd].running_days) > AVR_ADWD_LIMIT &&
		_method == MIGRATION_OURS) {
		flag = FLAG_DENY;
	}
	else if (flag == FLAG_SWAP &&
		((_SSD_list[_from_ssd].total_write_MB + _VIDEO_SEGMENT_list[_from_vid].size) / (_SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].DWPD) / _SSD_list[_from_ssd].running_days) > AVR_ADWD_LIMIT &&
		((_SSD_list[_to_ssd].total_write_MB + _VIDEO_SEGMENT_list[_to_vid].size) / (_SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].DWPD) / _SSD_list[_to_ssd].running_days) > AVR_ADWD_LIMIT &&
		_method == MIGRATION_OURS) {
		flag = FLAG_DENY;
	}

	return flag;
}