#include "header.h"

int run(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _method) {
	int migration_num = 0;
	switch (_method) {
	case MIGRATION_OURS:
		migration_num = our_algorithm(_SSD_list, _VIDEO_list);
		break;
	case MIGRATION_BANDWIDTH_AWARE:
		migration_num = benchmark(_SSD_list, _VIDEO_list);
		break;
	}
	return migration_num;
}


int our_algorithm(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	bool is_over_load[NUM_OF_SSDs + 1];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
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
		for (int to_ssd_temp = 1; to_ssd_temp <= NUM_OF_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				double slope_to = (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd_temp, from_vid)).second;
				double slope_from = (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd_temp, from_vid)).second;
				double slope = max(slope_to, slope_from);

				ADWD_after_list.insert(make_pair(slope, to_ssd_temp));
			}
		}

		int to_vid = -1;
		int to_ssd = -1;
		while (!ADWD_after_list.empty()) {
			to_ssd = (*ADWD_after_list.begin()).second;
			ADWD_after_list.erase(*ADWD_after_list.begin());
			if (to_ssd == from_ssd)
				continue;

			if (_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.size()) { // 옮겼을때 할당 가능한 경우들
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[to_ssd].storage_space)) && 
					((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) && 
					(_VIDEO_list[from_vid].requested_bandwidth > _VIDEO_list[to_vid].requested_bandwidth)) {
					break;
				}
				else if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) <= _SSD_list[to_ssd].storage_space)) && 
					(_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) {
					break;
				}
			}
			else { // 마이그레이션 할 ssd가 비었을 경우
				break;
			}
		}
		//할당 가능한 ssd 찾기, swap할 VIDEO도 찾기.
		if (ADWD_after_list.empty()) {
			for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
				printf("[SSD %d] bandwidth %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] storage %d / %d (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_space, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_space));
			}
			exit(0);
			//여기를 어떻게 할 지가 고민이야ㅠㅠ
		}

		//찾았으면 할당하기.
		if (to_vid != -1 && to_ssd != -1) {
			if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[to_ssd].storage_space)) &&
				((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth) <= _SSD_list[to_ssd].maximum_bandwidth)
				&& (_VIDEO_list[from_vid].requested_bandwidth > _VIDEO_list[to_vid].requested_bandwidth)) {
				
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin());
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[to_vid].requested_bandwidth, to_vid));
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
				_VIDEO_list[from_vid].assigned_SSD = to_ssd;
				_VIDEO_list[to_vid].assigned_SSD = from_ssd;
				_SSD_list[from_ssd].bandwidth_usage -= (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);
				_SSD_list[to_ssd].bandwidth_usage += (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);

				/*_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
				_SSD_list[from_ssd].storage_usage += _VIDEO_list[to_vid].size;
				_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
				_SSD_list[to_ssd].storage_usage -= _VIDEO_list[to_vid].size;*/
			}
			else if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) <= _SSD_list[to_ssd].storage_space)) && 
				(_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) {
				
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
				_VIDEO_list[from_vid].assigned_SSD = to_ssd;

				_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_list[from_vid].requested_bandwidth;
				_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
				_SSD_list[to_ssd].bandwidth_usage += _VIDEO_list[from_vid].requested_bandwidth;
				_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
			}
		}
		else {
			_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
			_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
			_VIDEO_list[from_vid].assigned_SSD = to_ssd;

			_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_list[from_vid].requested_bandwidth;
			_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
			_SSD_list[to_ssd].bandwidth_usage += _VIDEO_list[from_vid].requested_bandwidth;
			_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
		}

		_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid)).first;
		_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid)).first;
		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
		migration_num++;
	}

	/*for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	return migration_num;
}


int benchmark(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	bool is_over_load[NUM_OF_SSDs + 1];

	set<pair<double, int>, greater<pair<double, int>>> bandwidth_usage_of_SSDs;
	update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
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
		for (int to_ssd_temp = 1; to_ssd_temp <= NUM_OF_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				double remaining_bt;
				int to_vid_temp = (*_SSD_list[to_ssd_temp].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				if ((_SSD_list[to_ssd_temp].storage_usage + SIZE_OF_VIDEO) > _SSD_list[to_ssd_temp].storage_space) {
					remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid_temp].requested_bandwidth);
				}
				else {
					remaining_bt = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].bandwidth_usage - _VIDEO_list[from_vid].requested_bandwidth;
				}
				under_load_list.insert(make_pair(remaining_bt, to_ssd_temp));
			}
		}

		int to_vid = -1;
		int to_ssd = -1;
		while (!under_load_list.empty()) {
			to_ssd = (*under_load_list.begin()).second;
			under_load_list.erase(*under_load_list.begin());
			if (to_ssd == from_ssd)
				continue;

			if (_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.size()) { // 옮겼을때 할당 가능한 경우들
				to_vid = (*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
				if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[to_ssd].storage_space)) && ((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) && (_VIDEO_list[from_vid].requested_bandwidth > _VIDEO_list[to_vid].requested_bandwidth)) {
					break;
				}
				else if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) <= _SSD_list[to_ssd].storage_space)) && (_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) {
					break;
				}
			}
			else { // 마이그레이션 할 ssd가 비었을 경우
				break;
			}
		}
		//할당 가능한 ssd 찾기, swap할 VIDEO도 찾기.
		if (under_load_list.empty()) {
			for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
				printf("[SSD %d] %.2f / %.2f (%.2f%%)\n", ssd, _SSD_list[ssd].bandwidth_usage, _SSD_list[ssd].maximum_bandwidth, (_SSD_list[ssd].bandwidth_usage * 100 / _SSD_list[ssd].maximum_bandwidth));
				printf("[SSD %d] %d / %d (%.2f%%)\n\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_space, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_space));
			}
			exit(0);
			//여기를 어떻게 할 지가 고민이야ㅠㅠ
		}

		//찾았으면 할당하기.
		if (to_vid != -1 && to_ssd != -1) {
			if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[to_ssd].storage_space)) && ((_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) && (_VIDEO_list[from_vid].requested_bandwidth > _VIDEO_list[to_vid].requested_bandwidth)) {
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.begin());
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[to_vid].requested_bandwidth, to_vid));
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
				_VIDEO_list[from_vid].assigned_SSD = to_ssd;
				_VIDEO_list[to_vid].assigned_SSD = from_ssd;
				_SSD_list[from_ssd].bandwidth_usage -= (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);
				_SSD_list[to_ssd].bandwidth_usage += (_VIDEO_list[from_vid].requested_bandwidth - _VIDEO_list[to_vid].requested_bandwidth);

				/*_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
				_SSD_list[from_ssd].storage_usage += _VIDEO_list[to_vid].size;
				_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
				_SSD_list[to_ssd].storage_usage -= _VIDEO_list[to_vid].size;*/
			}
			else if ((((_SSD_list[to_ssd].storage_usage + SIZE_OF_VIDEO) <= _SSD_list[to_ssd].storage_space)) && (_SSD_list[to_ssd].bandwidth_usage + _VIDEO_list[from_vid].requested_bandwidth) < _SSD_list[to_ssd].maximum_bandwidth) {
				_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
				_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
				_VIDEO_list[from_vid].assigned_SSD = to_ssd;

				_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_list[from_vid].requested_bandwidth;
				_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
				_SSD_list[to_ssd].bandwidth_usage += _VIDEO_list[from_vid].requested_bandwidth;
				_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
			}
		}
		else {
			_SSD_list[from_ssd].assigned_VIDEOs_low_bandwidth_first.erase(element);
			_SSD_list[to_ssd].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[from_vid].requested_bandwidth, from_vid));
			_VIDEO_list[from_vid].assigned_SSD = to_ssd;

			_SSD_list[from_ssd].bandwidth_usage -= _VIDEO_list[from_vid].requested_bandwidth;
			_SSD_list[from_ssd].storage_usage -= SIZE_OF_VIDEO;
			_SSD_list[to_ssd].bandwidth_usage += _VIDEO_list[from_vid].requested_bandwidth;
			_SSD_list[to_ssd].storage_usage += SIZE_OF_VIDEO;
		}

		_SSD_list[to_ssd].ADWD += (get_slope_to(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid)).first;
		_SSD_list[from_ssd].ADWD += (get_slope_from(_SSD_list, _VIDEO_list, from_ssd, to_ssd, from_vid)).first;
		update_infomation(_SSD_list, is_over_load, &bandwidth_usage_of_SSDs);
		migration_num++;
	}

	/*for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	return migration_num;
}



void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs) {
	int _num_of_over_load = 0;
	(*_bandwidth_usage_of_SSDs).clear();
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		if (_SSD_list[ssd].bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			_is_over_load[ssd] = true;
			(*_bandwidth_usage_of_SSDs).insert(make_pair(_SSD_list[ssd].bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
			_num_of_over_load++;
		}
		else
			_is_over_load[ssd] = false;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}

pair<double, double> get_slope_to(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double ADWD_to, slope_to;
	double bt_difference;
	if ((_SSD_list[_to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[_to_ssd].storage_space) {
		int _to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[_to_vid].requested_bandwidth);
		ADWD_to = SIZE_OF_VIDEO / (_SSD_list[_to_ssd].storage_space * _SSD_list[_to_ssd].DWPD);
	}
	else {
		bt_difference = _VIDEO_list[_from_vid].requested_bandwidth;
		ADWD_to = SIZE_OF_VIDEO / (_SSD_list[_to_ssd].storage_space * _SSD_list[_to_ssd].DWPD);
	}
	slope_to = (_SSD_list[_to_ssd].ADWD + ADWD_to) / bt_difference;

	return make_pair(ADWD_to, slope_to);
}

pair<double, double> get_slope_from(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double ADWD_from, slope_from;
	double bt_difference;
	if ((_SSD_list[_to_ssd].storage_usage + SIZE_OF_VIDEO) > _SSD_list[_to_ssd].storage_space) {
		int _to_vid = (*_SSD_list[_to_ssd].assigned_VIDEOs_low_bandwidth_first.begin()).second;
		bt_difference = (_VIDEO_list[_from_vid].requested_bandwidth - _VIDEO_list[_to_vid].requested_bandwidth);
		ADWD_from = SIZE_OF_VIDEO / (_SSD_list[_to_ssd].storage_space * _SSD_list[_to_ssd].DWPD);
	}
	else {
		bt_difference = _VIDEO_list[_from_vid].requested_bandwidth;
		ADWD_from = 0;
	}
	slope_from = (_SSD_list[_from_ssd].ADWD + ADWD_from) / bt_difference;

	return make_pair(ADWD_from, slope_from);
}
