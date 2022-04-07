#include "header.h"
#define FLAG_REALLOCATE 0
#define FLAG_SWAP 1
#define FLAG_DENY -1

int migration(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos) {
	int* prev_SSD = new int[_num_of_videos];
	for (int vid = 0; vid < _num_of_videos; vid++) {
		prev_SSD[vid] = _VIDEO_CHUNK_list[vid].assigned_SSD;
	}

	int migration_num = 0;
	switch (_migration_method) {
	case MIGRATION_OURS:
		migration_num = migration_of_two_phase(_SSD_list, _VIDEO_CHUNK_list, _migration_method, _num_of_SSDs, _num_of_videos, prev_SSD);
		break;
	case MIGRATION_BANDWIDTH_AWARE:
	case MIGRATION_STORAGE_SPACE_AWARE:
	case MIGRATION_LIFETIME_AWARE:
	case MIGRATION_RANDOM:
	case MIGRATION_ROUND_ROBIN:
		migration_num = overload_elimination(_SSD_list, _VIDEO_CHUNK_list, _migration_method, _num_of_SSDs, _num_of_videos, prev_SSD);
		break;
	}

	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		set_serviced_video(_SSD_list, _VIDEO_CHUNK_list, _num_of_SSDs, _num_of_videos, ssd, true, &migration_num, prev_SSD);
	}
	//cout << migration_num << endl;
	return migration_num;
}

// �츮�� �˰����� vitual ssd�� ���ϵ��� �ű涧�� ������ ������ �Ҵ���. �� �� �뿪�� ��� ����.
// ���� �̷��ٰ� SSD�� ������ ���ٸ�, �� SSD�� �ٸ� SSD�� swap ����(�� target ssd�� ���� ���� ������ vitual�� �� ���� ����)
//                                                                 �׷��� swap���� (�Ű���) SSD ���� ���ٸ�, �� source SSD�� ���� �������� ���� ������.
// �׸��� �� SSD�� ���� ����ȭ�� �Ǿ��ٰ� �����ϰ�, �ٽô� �� SSD�� �Ҵ����� ����.
// �̷������� ��� ���� SSD�� �ִ� ���ϵ鿡 ���� SSD �ڸ��� ã���ָ鼭 ������. �� �� SSD�� ����ȭ�ǰų� underload��. overload�� ����.

int migration_of_two_phase(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos, int* _prev_SSD) {
	bool* is_over_load = new bool[_num_of_SSDs+1];
	bool* is_full = new bool[_num_of_SSDs+1];
	fill(is_full, is_full + _num_of_SSDs+1, false);

	set<pair<double, int>, greater<pair<double, int>>> over_load_SSDs; 
	update_SSD_infomation(_SSD_list, _VIDEO_CHUNK_list, _migration_method, is_over_load, is_full, &over_load_SSDs, _num_of_SSDs);
	//printf("num_of_over_load : %d\n", over_load_SSDs.size());
	//������� �ʱ�ȭ

	int migration_num = 0;
	bool visual_ssd_is_used = false;
	while (!over_load_SSDs.empty()) {
		int from_ssd = (*over_load_SSDs.begin()).second;
		if (from_ssd == VIRTUAL_SSD) {
			if(!visual_ssd_is_used)
				visual_ssd_is_used = true;
			if (_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.empty())
				break;
		}

		set<pair<double, int>>::iterator pos;
		pair<double, int> element;
		pos = _SSD_list[from_ssd].total_assigned_VIDEOs_low_bandwidth_first.end();
		element = (*--pos);
		int from_vid = element.second;

		//sort �ϱ�
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp] && !is_full[to_ssd_temp]) {
				int to_vid_temp = (*_SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				double bt;
				if (is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, to_ssd_temp, from_vid)) {
					bt = (_VIDEO_CHUNK_list[from_vid].requested_bandwidth - _VIDEO_CHUNK_list[to_vid_temp].requested_bandwidth);				
				}
				else {
					bt = _VIDEO_CHUNK_list[from_vid].requested_bandwidth;				
				}
				double ADWD = (_SSD_list[to_ssd_temp].total_write_MB + _VIDEO_CHUNK_list[from_vid].size) / (_SSD_list[to_ssd_temp].DWPD * _SSD_list[to_ssd_temp].storage_capacity * _SSD_list[to_ssd_temp].running_days);
				double remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].total_bandwidth_usage - bt) / _SSD_list[to_ssd_temp].maximum_bandwidth;

				if (bt < 0)
					continue;

				if (from_ssd != VIRTUAL_SSD) 
					under_load_list.insert(make_pair(bt / ADWD, to_ssd_temp));
				else
					under_load_list.insert(make_pair(remained_bandwidth / ADWD, to_ssd_temp));
			}
		}

		pair<int, pair<int, int>> mig_info = determine_migration_infomation(_SSD_list, _VIDEO_CHUNK_list, _migration_method, &under_load_list, from_ssd, from_vid);
		int to_vid = mig_info.second.second;
		int to_ssd = mig_info.second.first;
		int flag = mig_info.first;
		//if (v_flag && from_ssd != VIRTUAL_SSD && flag != FLAG_DENY)
		//int flag = get_migration_flag(_SSD_list, _VIDEO_CHUNK_list, MIGRATION_BANDWIDTH_AWARE, from_ssd, to_ssd, from_vid, to_vid);
		// ���⼭ �ɸ��°� ���ϱ�, BE ������ ���Ŀ� overload �Ǹ� �ٽ� �� SSD�� �ٽ� ���̱׷��̼��� �����Ѵ�. 
		// ���� �ű⼭�� �Ҵ��� �ȵǸ� stable ó���� �Ǹ鼭 ���� ���� �͵��� �������� ����ȭ ���ְ� ����. //20220325

		//ã������ �Ҵ��ϱ�.
		switch (flag) {
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_CHUNK_list, element, from_ssd, to_ssd, from_vid, to_vid, &migration_num, _prev_SSD);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_CHUNK_list, element, from_ssd, to_ssd, from_vid, &migration_num, _prev_SSD);
			break;
		case FLAG_DENY:
			if (under_load_list.empty()) {
				//������ �Ұ����� ��Ȳ�� ��� ��� �� ���ΰ�?�� ������ ���ʰ� ����.
				if (from_ssd == VIRTUAL_SSD) {
					_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.erase(element);
					_VIDEO_CHUNK_list[from_vid].assigned_SSD = NONE_ALLOC;
					_SSD_list[VIRTUAL_SSD].total_bandwidth_usage -= _VIDEO_CHUNK_list[from_vid].requested_bandwidth;
					_SSD_list[VIRTUAL_SSD].storage_usage -= _VIDEO_CHUNK_list[from_vid].size;
				}
				else {
					set_serviced_video(_SSD_list, _VIDEO_CHUNK_list, _num_of_SSDs, _num_of_videos, from_ssd, false, &migration_num, _prev_SSD);
					if (visual_ssd_is_used)
						is_full[from_ssd] = true;
				}
			}
			break;
		}

		update_SSD_infomation(_SSD_list, _VIDEO_CHUNK_list, _migration_method, is_over_load, is_full, &over_load_SSDs, _num_of_SSDs);
		if (flag != FLAG_DENY)
			migration_num++;
		under_load_list.clear();
		set<pair<double, int>, greater<pair<double, int>>>().swap(under_load_list); //�޸� ������ ����
	}

	delete[] is_over_load;

	over_load_SSDs.clear();
	set<pair<double, int>, greater<pair<double, int>>>().swap(over_load_SSDs);
	return migration_num;
}

int overload_elimination(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos, int* _prev_SSD){
	bool* is_over_load = new bool[_num_of_SSDs+1];
	default_random_engine g(SEED);
	uniform_int_distribution<> priority{ 1, _num_of_videos };
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>>> videos_in_over_load_SSDs;
	//for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		if (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			is_over_load[ssd] = true;
			set<pair<double, int>>::iterator pos = _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.begin();
			while (pos != _SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.end()) {
				switch (_migration_method) {
				case MIGRATION_BANDWIDTH_AWARE:
					videos_in_over_load_SSDs.insert(make_pair((*pos).first, make_pair((*pos).second, ssd)));
					break;
				case MIGRATION_STORAGE_SPACE_AWARE:
				case MIGRATION_LIFETIME_AWARE:
				case MIGRATION_RANDOM:
					videos_in_over_load_SSDs.insert(make_pair(priority(g), make_pair((*pos).second, ssd)));
					break;
				case MIGRATION_ROUND_ROBIN:
					videos_in_over_load_SSDs.insert(make_pair((double) 1/ (*pos).second, make_pair((*pos).second, ssd)));
					break;
				}
				pos++;
			}
		}
		else
			is_over_load[ssd] = false;
	}

	//������� �ʱ�ȭ
	int migration_num = 0;
	//bool visual_ssd_is_used = false;
	while (!videos_in_over_load_SSDs.empty()) {
		int from_ssd = (*videos_in_over_load_SSDs.begin()).second.second;
		int from_vid = (*videos_in_over_load_SSDs.begin()).second.first;
		pair<double, int> element = make_pair(_VIDEO_CHUNK_list[from_vid].requested_bandwidth, from_vid);
		videos_in_over_load_SSDs.erase(*videos_in_over_load_SSDs.begin());
		
		/*if (videos_in_over_load_SSDs.empty() && !visual_ssd_is_used) {
			is_over_load[VIRTUAL_SSD] = true;
			set<pair<double, int>>::iterator pos = _SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.begin();
			while (pos != _SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.end()) {
				videos_in_over_load_SSDs.insert(make_pair((*pos).first, make_pair((*pos).second, VIRTUAL_SSD)));
				pos++;
			}
			visual_ssd_is_used = true;
		}*/
		if (!is_over_load[from_ssd]) {
			continue;
		}

		//sort �ϱ�
		set<pair<double, int>, greater<pair<double, int>>> under_load_list;
		for (int to_ssd_temp = 1; to_ssd_temp <= _num_of_SSDs; to_ssd_temp++) {
			if (!is_over_load[to_ssd_temp]) {
				int to_vid_temp = (*_SSD_list[to_ssd_temp].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
				double bt;
				if (is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, to_ssd_temp, from_vid)) {
					bt = (_VIDEO_CHUNK_list[from_vid].requested_bandwidth - _VIDEO_CHUNK_list[to_vid_temp].requested_bandwidth);
				}
				else {
					bt = _VIDEO_CHUNK_list[from_vid].requested_bandwidth;
				}

				if (bt < 0)
					continue;

				double remained_bandwidth = (_SSD_list[to_ssd_temp].maximum_bandwidth - _SSD_list[to_ssd_temp].total_bandwidth_usage) / _SSD_list[to_ssd_temp].maximum_bandwidth;
				double remained_storage = (_SSD_list[to_ssd_temp].storage_capacity - _SSD_list[to_ssd_temp].storage_usage) / _SSD_list[to_ssd_temp].storage_capacity;
				uniform_int_distribution<> dist_priority{ 1, _num_of_SSDs };

				switch (_migration_method) {
				case MIGRATION_BANDWIDTH_AWARE:
					under_load_list.insert(make_pair(remained_bandwidth, to_ssd_temp));
					break;
				case MIGRATION_STORAGE_SPACE_AWARE:
					under_load_list.insert(make_pair(remained_storage, to_ssd_temp));
					break;
				case MIGRATION_LIFETIME_AWARE:
					under_load_list.insert(make_pair((double)1 / _SSD_list[to_ssd_temp].ADWD, to_ssd_temp));
					break;
				case MIGRATION_RANDOM:
					under_load_list.insert(make_pair(dist_priority(g), to_ssd_temp)); // �ű� SSD�� ���� �����ϱ� ����
					break;
				case MIGRATION_ROUND_ROBIN:
					under_load_list.insert(make_pair((double)1 / to_ssd_temp, to_ssd_temp));
					break;
				}
			}
		}

		pair<int, pair<int, int>> mig_info = determine_migration_infomation(_SSD_list, _VIDEO_CHUNK_list, _migration_method, &under_load_list, from_ssd, from_vid);
		int to_vid = mig_info.second.second;
		int to_ssd = mig_info.second.first;
		int flag = mig_info.first;

		//ã������ �Ҵ��ϱ�.
		//int flag = get_migration_flag(_SSD_list, _VIDEO_CHUNK_list, MIGRATION_BANDWIDTH_AWARE, from_ssd, to_ssd, from_vid, to_vid);
		switch (flag) {
		case FLAG_SWAP:
			swap(_SSD_list, _VIDEO_CHUNK_list, element, from_ssd, to_ssd, from_vid, to_vid, &migration_num, _prev_SSD);
			break;
		case FLAG_REALLOCATE:
			reallocate(_SSD_list, _VIDEO_CHUNK_list, element, from_ssd, to_ssd, from_vid, &migration_num, _prev_SSD);
			break;
		case FLAG_DENY:
			break;
		}

		update_SSD_infomation(_SSD_list, _VIDEO_CHUNK_list, _migration_method, is_over_load, NULL, NULL, _num_of_SSDs);
		if (flag != FLAG_DENY)
			migration_num++;
		under_load_list.clear();
		set<pair<double, int>, greater<pair<double, int>>>().swap(under_load_list); //�޸� ������ ����
	}
	videos_in_over_load_SSDs.clear();
	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>>>().swap(videos_in_over_load_SSDs);
	delete[] is_over_load;
	return migration_num;
}

pair<int, pair<int, int>> determine_migration_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, set<pair<double, int>, greater<pair<double, int>>>* under_load_list, int _from_ssd, int _from_vid) {
	int to_vid = NONE_ALLOC;
	int to_ssd = NONE_ALLOC;
	int flag = FLAG_DENY;
	while (!(*under_load_list).empty()) {
		to_ssd = (*(*under_load_list).begin()).second;
		(*under_load_list).erase(*(*under_load_list).begin());

		if (to_ssd == _from_ssd) {
			to_ssd = NONE_ALLOC;
			continue;
		}

		if (_SSD_list[to_ssd].total_assigned_VIDEOs_low_bandwidth_first.size()) { // �Ű����� �Ҵ� ������ ����
			to_vid = (*_SSD_list[to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
			flag = get_migration_flag(_SSD_list, _VIDEO_CHUNK_list, _migration_method, _from_ssd, to_ssd, _from_vid, to_vid);
		}
		else {
			flag = get_migration_flag(_SSD_list, _VIDEO_CHUNK_list, _migration_method, _from_ssd, to_ssd, _from_vid, NONE_ALLOC);
		}
		if (flag != FLAG_DENY) 
			break;
		to_ssd = NONE_ALLOC;
		to_vid = NONE_ALLOC;
	}

	return make_pair(flag, make_pair(to_ssd, to_vid));
}

void set_serviced_video(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, int ssd, bool flag, int* _migration_num, int* _prev_SSD) {
	if (_SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.empty()) {
		_SSD_list[ssd].total_bandwidth_usage = 0;
	}
	else {
		while (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			if (ssd == VIRTUAL_SSD)
				break;

			int vid = (*_SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.begin()).second;
			_SSD_list[ssd].total_bandwidth_usage -= _VIDEO_CHUNK_list[vid].requested_bandwidth;
			_SSD_list[ssd].storage_usage -= _VIDEO_CHUNK_list[vid].size;
			_SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());

			if (!flag) {
				_VIDEO_CHUNK_list[vid].assigned_SSD = VIRTUAL_SSD;
				_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_CHUNK_list[vid].requested_bandwidth, vid));
				_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _VIDEO_CHUNK_list[vid].requested_bandwidth;
				_SSD_list[VIRTUAL_SSD].storage_usage += _VIDEO_CHUNK_list[vid].size;
			}
			else {
				_VIDEO_CHUNK_list[vid].assigned_SSD = NONE_ALLOC;
			}

			//������ �ٸ� SSD���� �̹� �Ű����� �Ŷ��
			if (_prev_SSD[vid] != ssd && ssd != VIRTUAL_SSD) {
				_SSD_list[ssd].total_write_MB -= _VIDEO_CHUNK_list[vid].size;
				_SSD_list[ssd].ADWD = _SSD_list[ssd].total_write_MB / (_SSD_list[ssd].DWPD * _SSD_list[ssd].storage_capacity * _SSD_list[ssd].running_days);
				_migration_num--;
			}

			if (_SSD_list[ssd].total_assigned_VIDEOs_low_bandwidth_first.empty())
				break;
		}
	}
}

void swap(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid, int* _migration_num, int* _prev_SSD) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_CHUNK_list[_from_vid].size;
	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_CHUNK_list[_from_vid].requested_bandwidth;

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(*_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.begin());
	_SSD_list[_to_ssd].storage_usage -= _VIDEO_CHUNK_list[_to_vid].size;
	_SSD_list[_to_ssd].total_bandwidth_usage -= _VIDEO_CHUNK_list[_to_vid].requested_bandwidth;

	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_CHUNK_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_CHUNK_list[_from_vid].assigned_SSD = _to_ssd;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_CHUNK_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_CHUNK_list[_from_vid].size;

	_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_CHUNK_list[_to_vid].requested_bandwidth, _to_vid));
	_VIDEO_CHUNK_list[_to_vid].assigned_SSD = VIRTUAL_SSD;
	_SSD_list[VIRTUAL_SSD].total_bandwidth_usage -= _VIDEO_CHUNK_list[_to_vid].requested_bandwidth;
	_SSD_list[VIRTUAL_SSD].storage_usage += _VIDEO_CHUNK_list[VIRTUAL_SSD].size;

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_CHUNK_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = _SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].running_days);
	_SSD_list[VIRTUAL_SSD].total_write_MB += _VIDEO_CHUNK_list[_to_vid].size;
	_SSD_list[VIRTUAL_SSD].ADWD = _SSD_list[VIRTUAL_SSD].total_write_MB / (_SSD_list[VIRTUAL_SSD].DWPD * _SSD_list[VIRTUAL_SSD].storage_capacity * _SSD_list[_from_ssd].running_days);

	//������ �ٸ� SSD���� �̹� �Ű����� �Ŷ��
	if (_prev_SSD[_from_vid] != _from_ssd && _from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB -= _VIDEO_CHUNK_list[_from_vid].size;
		_SSD_list[_from_ssd].ADWD = _SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].running_days);
		_migration_num--;
	}
}

void reallocate(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int* _migration_num, int* _prev_SSD) {
	_SSD_list[_from_ssd].total_assigned_VIDEOs_low_bandwidth_first.erase(_element);
	_SSD_list[_to_ssd].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_CHUNK_list[_from_vid].requested_bandwidth, _from_vid));
	_VIDEO_CHUNK_list[_from_vid].assigned_SSD = _to_ssd;

	_SSD_list[_from_ssd].total_bandwidth_usage -= _VIDEO_CHUNK_list[_from_vid].requested_bandwidth;
	_SSD_list[_from_ssd].storage_usage -= _VIDEO_CHUNK_list[_from_vid].size;
	_SSD_list[_to_ssd].total_bandwidth_usage += _VIDEO_CHUNK_list[_from_vid].requested_bandwidth;
	_SSD_list[_to_ssd].storage_usage += _VIDEO_CHUNK_list[_from_vid].size;

	_SSD_list[_to_ssd].total_write_MB += _VIDEO_CHUNK_list[_from_vid].size;
	_SSD_list[_to_ssd].ADWD = _SSD_list[_to_ssd].total_write_MB / (_SSD_list[_to_ssd].DWPD * _SSD_list[_to_ssd].storage_capacity * _SSD_list[_to_ssd].running_days);

	//������ �ٸ� SSD���� �̹� �Ű����� �Ŷ��
	if (_prev_SSD[_from_vid] != _from_ssd && _from_ssd != VIRTUAL_SSD) {
		_SSD_list[_from_ssd].total_write_MB -= _VIDEO_CHUNK_list[_from_vid].size;
		_SSD_list[_from_ssd].ADWD = _SSD_list[_from_ssd].total_write_MB / (_SSD_list[_from_ssd].DWPD * _SSD_list[_from_ssd].storage_capacity * _SSD_list[_from_ssd].running_days);
		_migration_num--;
	}
}

void update_SSD_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, bool* _is_over_load, bool* _is_full, set<pair<double, int>, greater<pair<double, int>>>* _over_load_SSDs, int _num_of_SSDs) {
	if (_over_load_SSDs != NULL)
		(*_over_load_SSDs).clear();
	for (int ssd = 1; ssd <= _num_of_SSDs; ssd++) {
		if (_is_full != NULL) {
			if (_is_full[ssd])
				continue;
		}
		if (_SSD_list[ssd].total_bandwidth_usage > _SSD_list[ssd].maximum_bandwidth) {
			_is_over_load[ssd] = true;
			if (_over_load_SSDs != NULL) {
				(*_over_load_SSDs).insert(make_pair(_SSD_list[ssd].total_bandwidth_usage - _SSD_list[ssd].maximum_bandwidth, ssd));
			}
		}
		else
			_is_over_load[ssd] = false;
	}
	//���� �����ε� �� SSD�� ���ٸ�
	if (_over_load_SSDs != NULL) {
		if ((*_over_load_SSDs).empty()) {
			(*_over_load_SSDs).insert(make_pair(-INFINITY, VIRTUAL_SSD));
		}
	}
}

int get_migration_flag(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid) {
	int flag = FLAG_DENY;

	if (_to_vid == NONE_ALLOC) {
		flag = FLAG_REALLOCATE;
	}
	else {
		if (_from_ssd == VIRTUAL_SSD && _migration_method == MIGRATION_OURS) {
			if (!is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, _to_ssd, _from_vid)) {
				flag = FLAG_REALLOCATE; // HDD�� �α⵵ ���� ������ �ִ��� ������ �ʱ� ����
			}
			else {
				flag = FLAG_SWAP;
			}
		}
		else {
			if (!is_full_storage_space(_SSD_list, _VIDEO_CHUNK_list, _to_ssd, _from_vid)) {
				if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_CHUNK_list[_from_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth)
					flag = FLAG_REALLOCATE; // SSD�� �α⵵ ���� ������ ��� ����� ����
			}
			else {
				if ((_SSD_list[_to_ssd].total_bandwidth_usage + _VIDEO_CHUNK_list[_from_vid].requested_bandwidth - _VIDEO_CHUNK_list[_to_vid].requested_bandwidth) <= _SSD_list[_to_ssd].maximum_bandwidth)
					flag = FLAG_SWAP;
			}
		}
	}

	return flag;
}