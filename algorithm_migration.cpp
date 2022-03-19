#include "header.h"
#define FLAG_REALLOCATE 0
#define FLAG_SWAP 1
#define FLAG_DENY -1

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	int migration_num = 0;
	switch (_migration_method) {
	case MIGRATION_OURS:
	case MIGRATION_THROUGHPUT_AWARE:
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
		pair<double, int> element = make_pair((*videos_in_over_load_SSDs.begin()).first, from_vid);
		videos_in_over_load_SSDs.erase(*videos_in_over_load_SSDs.begin());
		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int to_vid_temp = (*_SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				double difference_bt, ADWD, remained_bandwidth, remained_storage;
				if (is_swap(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, from_vid)) {
					difference_bt = (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth);
					ADWD = ((_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / _SSD_list[to_ssd_temp].running_days) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity);
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth)));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				}
				else {
					difference_bt = (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth);
					ADWD = ((_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / _SSD_list[to_ssd_temp].running_days) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity);
					remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - (_SSD_list[to_ssd_temp].total_bandwidth_usage + _VIDEO_SEGMENT_list[from_vid].requested_bandwidth));
					remained_storage = _SSD_list[to_ssd_temp].storage_capacity - (_SSD_list[to_ssd_temp].storage_usage + _VIDEO_SEGMENT_list[from_vid].size);
				}
				double slope = remained_bandwidth / ADWD;
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
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid);
			videos_in_over_load_SSDs.insert(make_pair(_VIDEO_SEGMENT_list[to_vid].requested_bandwidth, make_pair(to_vid, VIRTUAL_SSD)));
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid);
			break;
		case FLAG_DENY:
			if (from_ssd == VIRTUAL_SSD) {
				_VIDEO_SEGMENT_list[from_vid].assigned_SSD = NONE_ALLOC;
				_SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(make_pair(_VIDEO_SEGMENT_list[from_vid].requested_bandwidth, from_vid));
				_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
				_SSD_list[from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
			}
		}
		update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, _num_of_SSDs, &videos_in_over_load_SSDs, is_over_load);
		migration_num++;
		under_load_list.clear();
		set<pair<double, int>, greater<pair<double, int>>>().swap(under_load_list); //메모리 해제를 위해
	}

	delete[] is_over_load;
	return migration_num;
}

void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());
	_SSD_list[_to_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage -= _VIDEO_SEGMENT_list[_to_vid].size;
	//두 파일을 각각 삭제

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid)); //주석 처리 이유 : 옮긴 비디오를 또 옮기지 못하게 막으려는 의도. 어차피 비디오 인기도 업데이트 되면, initialization때 다시 total_assigned_VIDEOs_low_bandwidth_first안에 비디오 넣는 작업 한다.
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_SEGMENT_list[_from_vid].size;
	//source SSD의 파일을 target으로 이동.

	//target에서 뺀걸 Virtual SSD에 넣는다. 
	if (_from_ssd != VIRTUAL_SSD) {
		_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_to_vid].requested_bandwidth, _to_vid)); //주석 처리 이유 : 옮긴 비디오를 또 옮기지 못하게 막으려는 의도. 어차피 비디오 인기도 업데이트 되면, initialization때 다시 total_assigned_VIDEOs_low_bandwidth_first안에 비디오 넣는 작업 한다.
		_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = VIRTUAL_SSD;
		_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth;
		_SSD_list[VIRTUAL_SSD].storage_usage += _VIDEO_SEGMENT_list[_to_vid].size;
	}
	else {
		_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = NONE_ALLOC;
	}

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = (_SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity)) / _SSD_list[_to_ssd].running_days;
}

void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	//_VIDEO_SEGMENT_list[from_vid].is_serviced = true;

	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_SEGMENT_list[_from_vid].size;

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = (_SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity)) / _SSD_list[_to_ssd].running_days;
}

/*void update_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, bool* _is_over_load, int _num_of_SSDs) {
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			_is_over_load[ssd] = true;
		}
		else
			_is_over_load[ssd] = false;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
}*/

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



int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	int flag = FLAG_DENY;

	if (_to_vid != NONE_ALLOC) {
		if (!is_swap(_SSD_list, _VIDEO_SEGMENT_list, _to_ssd, _from_vid)) {
			if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth)
				flag = FLAG_REALLOCATE;
		}
		else {
			if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth &&
				_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth > _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth) {
				flag = FLAG_SWAP;
			}
		}
	}
	else { // 여기로 오는 경우는 _to_ssd가 빈 경우
		flag = FLAG_REALLOCATE;
	}
	return flag;
}