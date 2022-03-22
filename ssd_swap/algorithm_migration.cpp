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

	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		set_serviced_video(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_videos, ssd, true);
	}
	return migration_num;
}

int migration_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	int* prev_SSD = new int[_num_of_videos];
	for (int vid = 0; vid < _num_of_videos; vid++) {
		prev_SSD[vid] = _VIDEO_SEGMENT_list[vid].assigned_SSD;
	}

	bool* is_over_load = new bool[_num_of_SSDs+1];
	bool* is_imposible = new bool[_num_of_SSDs+1];
	fill(is_imposible, is_imposible + _num_of_SSDs+1, false);

	set<pair<double, int>, greater<pair<double, int>>> over_load_SSDs; 
	update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs);
	//printf("num_of_over_load : %d\n", over_load_SSDs.size());
	//여기까지 초기화

	int migration_num = 0;
	while (!over_load_SSDs.empty()) {
		int from_ssd = (*over_load_SSDs.begin()).second;
		set<pair<double, int>>::iterator pos;
		pair<double, int> element;
		pos = _SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.end();
		element = (*--pos);
		int from_vid = element.second;

		//sort 하기
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int to_vid_temp = (*_SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				double bt;
				double st;
				if (is_full_storage_space(_SSD_list, _VIDEO_SEGMENT_list, to_ssd_temp, from_vid)) {
					bt = (_VIDEO_SEGMENT_list[from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[to_vid_temp].requested_bandwidth);
					st = 0;
				}
				else {
					bt = _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
					st = _VIDEO_SEGMENT_list[from_vid].size;
				}
				double ADWD = (_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_SEGMENT_list[from_vid].size) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity * _SSD_list[to_ssd_temp].running_days);
				double remained_bandwidth = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].total_bandwidth_usage;
				double remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				double ADWD_from = (_SSD_list[from_ssd].total_write_MB + _VIDEO_SEGMENT_list[to_vid_temp].size) / (_SSD_list[from_ssd].DWPD * _SSD_list[from_ssd].storage_capacity * _SSD_list[from_ssd].running_days);

				//double ADWD = _SSD_list[to_ssd_temp].ADWD;
				//double remained_bandwidth = _SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].total_bandwidth_usage;
				//double remained_storage = _SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage;
				if (bt < 0)
					continue;

				//bt /= (_SSD_list[to_ssd_temp].total_bandwidth_usage + bt); // 어떤 상황에서도 이게 bt만 하는 것 보다 나음.
				//bt /= _SSD_list[to_ssd_temp].maximum_bandwidth랑, 그냥 bt 둘 다 별로임
				double slope = -INFINITY;
				switch (_migration_method)
				{
				case MIGRATION_OURS:
					//remained_storage -= st;
					//slope = (bt * remained_storage) / ADWD;
					slope = bt / max(ADWD, ADWD_from);
					break;
				case MIGRATION_BANDWIDTH_AWARE:
					slope = remained_bandwidth;
					break;
				case MIGRATION_STORAGE_SPACE_AWARE:
					slope = remained_storage;
					break;
				case MIGRATION_LIFETIME_AWARE:
					slope = 1/ADWD;
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
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, to_vid, prev_SSD);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_SEGMENT_list, element, from_ssd, to_ssd, from_vid, prev_SSD);
			break;
		case FLAG_DENY:
			if (under_load_list.empty()) {
				//스왑이 불가능한 상황일 경우 어떻게 할 것인가?를 생각할 차례가 왔음.
				if (from_ssd == VIRTUAL_SSD && _migration_method == MIGRATION_OURS && _SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.size() > 1) {
					_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.erase(element);
					_VIDEO_SEGMENT_list[from_vid].assigned_SSD = NONE_ALLOC;
					_SSD_list[VIRTUAL_SSD].total_bandwidth_usage -= _VIDEO_SEGMENT_list[from_vid].requested_bandwidth;
					_SSD_list[VIRTUAL_SSD].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
				}
				else {
					if (from_ssd != VIRTUAL_SSD && _migration_method == MIGRATION_OURS) {
						set_serviced_video(_SSD_list, _VIDEO_SEGMENT_list, _num_of_SSDs, _num_of_videos, from_ssd, false);
					}
					is_imposible[from_ssd] = true;
					update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs);
				}
				continue;
			}
			break;
		}

		update_infomation(_SSD_list, _VIDEO_SEGMENT_list, _migration_method, is_over_load, is_imposible, &over_load_SSDs, _num_of_SSDs);
		migration_num++;
		under_load_list.clear();
		set<pair<double, int>, greater<pair<double, int>>>().swap(under_load_list); //메모리 해제를 위해
	}

	delete[] is_over_load;

	over_load_SSDs.clear();
	set<pair<double, int>, greater<pair<double, int>>>().swap(over_load_SSDs);
	return migration_num;
}

void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid, int* _prev_SSD) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;
	_SSD_list[_to_ssd].total_bandwidth_usage += (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);

	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_to_vid].requested_bandwidth, _to_vid));
	_VIDEO_SEGMENT_list[_to_vid].assigned_SSD = _from_ssd;
	_SSD_list[_from_ssd].total_bandwidth_usage -= (_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth - _VIDEO_SEGMENT_list[_to_vid].requested_bandwidth);

	/*_SSD_list[from_ssd].storage_usage -= _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[from_ssd].storage_usage += _VIDEO_SEGMENT_list[to_vid].size;
	_SSD_list[to_ssd].storage_usage += _VIDEO_SEGMENT_list[from_vid].size;
	_SSD_list[to_ssd].storage_usage -= _VIDEO_SEGMENT_list[to_vid].size;*/

	if (_to_ssd != VIRTUAL_SSD) {
		_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
		_SSD_list[_to_ssd].ADWD = _SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].running_days);
	}
	// 알고리즘에서 다른 데 할당했다가 또 할당한 것일때.
	if (_prev_SSD[_from_vid] != _from_ssd && _from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB -= _VIDEO_SEGMENT_list[_from_ssd].size;
		_SSD_list[_from_ssd].ADWD = _SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].running_days);
	}
	if (_from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB += _VIDEO_SEGMENT_list[_to_vid].size;
		_SSD_list[_from_ssd].ADWD = _SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].running_days);
	}
}

void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int* _prev_SSD) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_SEGMENT_list[_from_vid].assigned_SSD = _to_ssd;

	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_SEGMENT_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_SEGMENT_list[_from_vid].size;

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_SEGMENT_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = _SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].running_days);

	// 알고리즘에서 다른 데 할당했다가 또 할당한 것일때.
	if (_prev_SSD[_from_ssd] != _from_ssd && _from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB -= _VIDEO_SEGMENT_list[_from_vid].size;
		_SSD_list[_from_ssd].ADWD = _SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].running_days);
	}
}

void update_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, bool* _is_over_load, bool* _is_imposible, set<pair<double, int>, greater<pair<double, int>>>* _over_load_SSDs, int _num_of_SSDs) {
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
			switch (_migration_method)
			{
			case MIGRATION_OURS: 
				(*_over_load_SSDs).insert(make_pair(_SSD_list[ssd].total_bandwidth_usage / _SSD_list[ssd].maximum_bandwidth, ssd));
				break;
			case MIGRATION_BANDWIDTH_AWARE:
				(*_over_load_SSDs).insert(make_pair(_SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
				break;
			case MIGRATION_STORAGE_SPACE_AWARE:
				(*_over_load_SSDs).insert(make_pair(_SSD_list[ssd].storage_capacity - _SSD_list[ssd].storage_usage, ssd));
				break;
			case MIGRATION_LIFETIME_AWARE:
				(*_over_load_SSDs).insert(make_pair(1/_SSD_list[ssd].ADWD, ssd));
				break;
			}

			_num_of_over_load++;
		}
		else
			_is_over_load[ssd] = false;
	}
	//printf("num_of_over_load : %d\n", _num_of_over_load);
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