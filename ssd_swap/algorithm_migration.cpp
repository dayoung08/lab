#include "header.h"
#define FLAG_ALLOCATE 0
#define FLAG_REPLACE 1
#define FLAG_DENY -1

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	int migration_num = 0;
	switch (_migration_method) {
	case MIGRATION_OURS:
		migration_num = migration_resource_aware(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, _num_of_SSDs, _num_of_videos);
		break;
	case MIGRATION_THROUGHPUT_AWARE:
	case MIGRATION_BANDWIDTH_AWARE:
	case MIGRATION_STORAGE_SPACE_AWARE:
	case MIGRATION_LIFETIME_AWARE:
		migration_num = migration_for_benchmark(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, _num_of_SSDs, _num_of_videos);
		set_serviced_video(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_videos);
		break;
	}
	return migration_num;
}

int migration_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	bool* is_over_load = new bool[_num_of_SSDs + 1];
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>>> videos_in_over_load_SSDs;
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			is_over_load[ssd] = true;
			set<pair<double, int>>::iterator pos = _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.begin();
			while (pos != _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.end()) {
				videos_in_over_load_SSDs.insert(make_pair((*pos).first, make_pair((*pos).second, ssd)));
				pos++;
			}
		}
		else
			is_over_load[ssd] = false;
	}

	//여기까지 초기화
	int migration_num = 0;
	while (!videos_in_over_load_SSDs.empty()) {
		int from_ssd = (*videos_in_over_load_SSDs.begin()).second.second;
		int from_vid = (*videos_in_over_load_SSDs.begin()).second.first;
		//is_processed_video[from_vid] = true;
		pair<double, int> element = make_pair((*videos_in_over_load_SSDs.begin()).first, from_vid);
		videos_in_over_load_SSDs.erase(*videos_in_over_load_SSDs.begin());
		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				set<pair<double, int>>::iterator pos = _SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin();
				int to_vid_temp = (*pos).second;

				//double slope_to = get_slope_to(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid);
				//double slope_from = get_slope_from(_SSD_list, _VIDEO_SEGMENT_list, from_ssd, to_ssd_temp, from_vid);
				double ADWD = ((_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / _SSD_list[to_ssd_temp].running_days) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity);
				double remained_bandwidth;
				double remained_storage;
				double difference_bt;

				if (is_replaced(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, to_vid_temp)) {
					difference_bt = (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth);
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth)));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				}
				else {
					difference_bt = (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth);
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + _VIDEO_SEGMENT_list[from_vid].requested_bandwidth));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - (_SSD_list[to_ssd_temp].storage_usage + _VIDEO_SEGMENT_list[from_vid].size);
				}

				double slope = -INFINITY;
				switch (_migration_method)
				{
				case MIGRATION_OURS:
					//slope = slope_to;
					slope = remained_bandwidth / ADWD;
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

		//찾았으면 할당하기.
		//int flag = get_migration_flag(_SSD_list, _VIDEO_SEGMENT_list, MIGRATION_BANDWIDTH_AWARE, from_ssd, to_ssd, from_vid, to_vid);
		switch (flag) {
		case FLAG_REPLACE:
			migration_and_elimination(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			videos_in_over_load_SSDs.insert(make_pair(_VIDEO_SEGMENT_list[to_vid].requested_bandwidth, make_pair(to_vid, VIRTUAL_SSD)));
			break;
		case FLAG_ALLOCATE:
			allocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		case FLAG_DENY:
			_VIDEO_SEGMENT_list[from_vid].assigned_SSD = NONE_ALLOC;
			break;
		}

		update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, _num_of_SSDs, &videos_in_over_load_SSDs, is_over_load);
		if (flag != FLAG_DENY)
			migration_num++;
	}
	/*for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
		printf("[SSD bandwidth %d] %.2f / %.2f (%.2f%%)\n", ssd, SSD_list[ssd].bandwidth_usage, SSD_list[ssd].maximum_bandwidth, (SSD_list[ssd].bandwidth_usage * 100 / SSD_list[ssd].maximum_bandwidth));
		//printf("[SSD storage %d] %d / %d (%.2f%%)\n", ssd, SSD_list[ssd].storage_usage, SSD_list[ssd].storage_space, ((double)SSD_list[ssd].storage_usage * 100 / SSD_list[ssd].storage_space));
	}*/
	delete[] is_over_load;
	return migration_num;
}


int migration_for_benchmark(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	bool* is_over_load = new bool[_num_of_SSDs + 1];
	bool* is_imposible = new bool[_num_of_SSDs + 1];
	fill(is_imposible, is_imposible + _num_of_SSDs, false);

	set<pair<double, int>, greater<pair<double, int>>> over_load_SSDs;
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].total_bandwidth_usage <= _SSD_list[ssd].maximum_bandwidth) {
			is_over_load[ssd] = false;
		}
		else {
			if(ssd == VIRTUAL_SSD)
				over_load_SSDs.insert(make_pair(INFINITY, ssd));
			else {
				double throuput = _SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth;
				over_load_SSDs.insert(make_pair(throuput, ssd));
			}
			is_over_load[ssd] = true;
		}
	}

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
				set<pair<double, int>>::iterator pos = _SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin();
				int to_vid_temp = (*pos).second;

				double ADWD = ((_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / _SSD_list[to_ssd_temp].running_days) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity);
				double remained_bandwidth = 0;
				double remained_storage = 0;

				if (is_replaced(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, to_vid_temp)) {
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth)));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				}
				else {
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + _VIDEO_SEGMENT_list[from_vid].requested_bandwidth));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - (_SSD_list[to_ssd_temp].storage_usage + _VIDEO_SEGMENT_list[from_vid].size);
				}

				if (remained_bandwidth < 0 || remained_storage < 0)
					continue;

				double slope = -INFINITY;
				switch (_migration_method)
				{
				case MIGRATION_THROUGHPUT_AWARE:
					slope = (remained_bandwidth / remained_storage);
					break;
				case MIGRATION_BANDWIDTH_AWARE:
					slope = remained_bandwidth;
					break;
				case MIGRATION_STORAGE_SPACE_AWARE:
					slope = remained_storage;
					break;
				case MIGRATION_LIFETIME_AWARE:
					slope = 1 - ADWD;
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

		switch (flag) {
		case FLAG_REPLACE:
			migration_and_elimination(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			break;
		case FLAG_ALLOCATE:
			allocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		case FLAG_DENY:
			if (under_load_list.empty()) {
				is_imposible[from_ssd] = true;
				if (from_ssd == VIRTUAL_SSD) {
					while (!_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.empty()) {
						int vid = (*_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
						_VIDEO_SEGMENT_list[vid].assigned_SSD = NONE_ALLOC;
						_SSD_list[from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[vid].requested_bandwidth;
						_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());
					}
				}
			}
			break;
		}
		update_infomation_for_benchmark(_SSD_list, is_over_load, &over_load_SSDs, _num_of_SSDs, is_imposible);
		if (flag != FLAG_DENY) 
			migration_num++;
	}
	delete[] is_over_load;

	return migration_num;
}


void migration_and_elimination(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());
	//두 삭제

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid)); //주석 처리 이유 : 옮긴 비디오를 또 옮기지 못하게 막으려는 의도. 어차피 비디오 인기도 업데이트 되면, initialization때 다시 total_assigned_VIDEOs_low_bandwidth_first안에 비디오 넣는 작업 한다.
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	_SSD_list[_to_ssd].total_bandwidth_usage += (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
	//source SSD의 파일을 target으로 이동.

	//target에서 뺀걸 Virtual SSD에 넣는다. 
	if (_from_ssd != VIRTUAL_SSD) {
		_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_to_vid].requested_bandwidth, _to_vid)); //주석 처리 이유 : 옮긴 비디오를 또 옮기지 못하게 막으려는 의도. 어차피 비디오 인기도 업데이트 되면, initialization때 다시 total_assigned_VIDEOs_low_bandwidth_first안에 비디오 넣는 작업 한다.
		_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = VIRTUAL_SSD;
		_SSD_list[VIRTUAL_SSD].total_bandwidth_usage -= (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
	}

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = (_SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity)) / _SSD_list[_to_ssd].running_days;
}

void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid)); //주석 처리 이유 : 옮긴 비디오를 또 옮기지 못하게 막으려는 의도. 어차피 비디오 인기도 업데이트 되면, initialization때 다시 total_assigned_VIDEOs_low_bandwidth_first안에 비디오 넣는 작업 한다.

	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;

	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_SEGMENT_list[_from_vid].size;

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = (_SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity)) / _SSD_list[_to_ssd].running_days;
}

void update_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, bool* _is_over_load, int _num_of_SSDs, set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>>>* videos_in_over_load_SSDs, bool* is_over_load) {
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].total_bandwidth_usage <= _SSD_list[ssd].maximum_bandwidth) { // overload에서 빠져나옴
			if (is_over_load[ssd]) {
				set<pair<double, int>>::iterator pos = _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.begin();
				while (pos != _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.end()) {
					(*videos_in_over_load_SSDs).erase(make_pair((*pos).first, make_pair((*pos).second, ssd)));
					pos++;
				}
			}
			is_over_load[ssd] = false;
		}
		else
			is_over_load[ssd] = true;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}

void update_infomation_for_benchmark(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _over_load_SSDs, int _num_of_SSDs, bool* _is_imposible) {
	set<pair<double, int>>::iterator pos = (*_over_load_SSDs).begin();
	while (pos != (*_over_load_SSDs).end()) {
		int ssd = (*pos).second;
		(*_over_load_SSDs).erase(*pos);
		if (_SSD_list[ssd].total_bandwidth_usage <= _SSD_list[ssd].maximum_bandwidth) {
			_is_over_load[ssd] = false;
		}
		else {
			if (ssd == VIRTUAL_SSD && _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.empty()) {
				_is_over_load[ssd] = false;
			}
			else {
				if (!_is_imposible[ssd]) {
					double throuput = _SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth;
					(*_over_load_SSDs).insert(make_pair(throuput, ssd));
				}
			}
		}
		pos++;
	}
	if ((*_over_load_SSDs).empty() && _SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.size()) {
		(*_over_load_SSDs).insert(make_pair(INFINITY, VIRTUAL_SSD));
	}
}

int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	int flag = FLAG_DENY;

	if (_to_vid != NONE_ALLOC) {
		if (!is_replaced(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
			if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth)
				flag = FLAG_ALLOCATE;
		}
		else {
			if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth &&
				_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth > _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) {
				flag = FLAG_REPLACE;
			}
		}
	}
	else { // 여기로 오는 경우는 _to_ssd가 빈 경우
		flag = FLAG_ALLOCATE;
	}
	return flag;
}
/*
double get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double difference_bt;
	double slope_to;
	double ADWD_to = ((_SSD_list[_to_ssd].total_write_MB + _VIDEO_SEGMENT_list[_from_vid].size) / _SSD_list[_to_ssd].running_days) / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity);
	double remained_bandwidth;
	double remained_storage;
	//double remained_write_MB;

	if (is_swap(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
		remained_bandwidth = (_SSD_list[_to_ssd].maximum_bandwidth - (_SSD_list[_to_ssd].total_bandwidth_usage + (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth)));
		remained_storage = _SSD_list[_to_ssd].storage_capacity - _SSD_list[_to_ssd].storage_usage;
	}
	else {
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth);
		remained_bandwidth = (_SSD_list[_to_ssd].maximum_bandwidth - (_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth));
		remained_storage = _SSD_list[_to_ssd].storage_capacity - (_SSD_list[_to_ssd].storage_usage + _VIDEO_SEGMENT_list[_from_vid].size);
	}

	slope_to = difference_bt / ADWD_to;
	//slope_to = (remained_bandwidth / remained_storage) / ADWD_to;
	return slope_to;
}*/
/*
double get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid) {
	double difference_bt;
	double slope_from;
	double ADWD_from;				
	double remained_bandwidth = _SSD_list[_to_ssd].maximum_bandwidth - _SSD_list[_to_ssd].total_bandwidth_usage;
	double remained_storage = _SSD_list[_to_ssd].storage_capacity - _SSD_list[_to_ssd].storage_usage;

	if (is_swap(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
		int _to_vid = (*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth); 
		ADWD_from = ((_SSD_list[_from_ssd].total_write_MB + _VIDEO_SEGMENT_list[_to_vid].size) / _SSD_list[_from_ssd].running_days) / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity);
		remained_bandwidth -= (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);
	}
	else {
		difference_bt = (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth);
		ADWD_from = (_SSD_list[_from_ssd].total_write_MB / _SSD_list[_from_ssd].running_days) / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity);
		remained_bandwidth -= _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
		remained_storage -= _VIDEO_SEGMENT_list[_from_vid].size;
	}
	slope_from = difference_bt / ADWD_from;
	//slope_from = (remained_bandwidth / remained_storage) / ADWD_from;
	return slope_from;
}
*/