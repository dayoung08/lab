#include "header.h"

int rand_cnt_for_placement = 0; //for generation seed in placement_random
int placement(SSD* _SSD_list, VIDEO* _VIDEO_list, int _method) {
	int placement_num = 0;
	switch (_method) {
	case PLACEMENT_OURS:
		placement_num = placement_myAlgorithm(_SSD_list, _VIDEO_list);
		break;
	case PLACEMENT_RANDOM:
		placement_num = placement_random(_SSD_list, _VIDEO_list);
		break;
	}
	return placement_num;
}

int placement_myAlgorithm(SSD* _SSD_list, VIDEO* _VIDEO_list) {
	int placement_num = 0;
	
	return placement_num;
}

int placement_random(SSD* _SSD_list, VIDEO* _VIDEO_list) {
	int placement_num = 0;
	std::mt19937 g(SEED + rand_cnt_for_placement);
	rand_cnt_for_placement++;
	vector<int> not_full_ssd_list(NUM_OF_SSDs);
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		not_full_ssd_list[ssd - 1] = ssd;
	}
	std::shuffle(not_full_ssd_list.begin(), not_full_ssd_list.end(), g);

	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		int ssd_index = -1;

		while (!not_full_ssd_list.empty()) {
			ssd_index = not_full_ssd_list.back();
			if (is_not_enough_storage_space(_SSD_list, _VIDEO_list, ssd_index, video_index))
				not_full_ssd_list.pop_back();
			else {
				allocate(_SSD_list, _VIDEO_list, video_index, ssd_index);
				placement_num++;
				break;
			}
		}
	}
	return placement_num;
}

void allocate(SSD* _SSD_list, VIDEO* _VIDEO_list, int _video_index, int _ssd_index) {
	_VIDEO_list[_video_index].assigned_SSD = _ssd_index;
	_SSD_list[_ssd_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[_video_index].requested_bandwidth, _video_index));

	_SSD_list[_ssd_index].storage_usage += _VIDEO_list[_video_index].size;
	_SSD_list[_ssd_index].bandwidth_usage += _VIDEO_list[_video_index].requested_bandwidth;

	_VIDEO_list[_video_index].is_alloc = true;
}