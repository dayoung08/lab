#include "header.h"


double total_maximum_bandwidth = 0;
double* seg_pop;

void initalization(SSD* _SSD_list, video_segment* _segment_list) {
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].index = index;
		_SSD_list[index].DWPD = ((double)(rand() % (MAX_DWPD+1) + MIN_DWPD)) / 100;
		_SSD_list[index].storage_space = 2000; // 용량 각각 다 다르게 하니 제대로 안 돌아감
		//_SSD_list[index].storage_space = 0.5 * pow(2, (rand() % 8)); // 0.5, 1, 2, 4, 8. 16. 32TB

		_SSD_list[index].maximum_bandwidth = rand() % (MAX_SSD_BANDWIDTH - MIN_SSD_BANDWIDTH + 1) + MIN_SSD_BANDWIDTH;
		total_maximum_bandwidth += _SSD_list[index].maximum_bandwidth; 
		//https://tekie.com/blog/hardware/ssd-vs-hdd-speed-lifespan-and-reliability/
		//https://www.quora.com/What-is-the-average-read-write-speed-of-an-SSD-hard-drive

		_SSD_list[index].ADWD = 0;
		_SSD_list[index].storage_usage = 0;
		_SSD_list[index].bandwidth_usage = 0;
		//_SSD_list[index].MB_write = 0;
	}

	total_maximum_bandwidth *= 0.8;
	seg_pop = set_zipf_pop(NUM_OF_SEGMENTs, ALPHA, BETA);
	vector<double>seg_pop_shuffle(seg_pop, seg_pop + NUM_OF_SEGMENTs);
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(seg_pop_shuffle.begin(), seg_pop_shuffle.end(), g);

	vector<int> not_full_ssd_list(NUM_OF_SSDs);
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		not_full_ssd_list[ssd - 1] = ssd;
	}
	for (int vid = 1; vid <= NUM_OF_SEGMENTs; vid++) {
		int index = vid;
		_segment_list[index].index = index;
		_segment_list[index].size = SIZE_OF_SEGMENT;

		double bandwidth = seg_pop_shuffle[NUM_OF_SEGMENTs - vid];
		seg_pop_shuffle.pop_back();
		_segment_list[index].requested_bandwidth = bandwidth * total_maximum_bandwidth; //0913 수정
		
		int SSD_index = -1;
		std::shuffle(not_full_ssd_list.begin(), not_full_ssd_list.end(), g);
		while (true) {
			if (not_full_ssd_list.empty()) {
				printf("모든 용량이 꽉 차서 저장할 만한 SSD가 없음\n");
				/*for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
					printf("[SSD %d] %d / %d (%.2f%%)\n", ssd, _SSD_list[ssd].storage_usage, _SSD_list[ssd].storage_space, ((double)_SSD_list[ssd].storage_usage * 100 / _SSD_list[ssd].storage_space));
				}*/
				exit(0);
			}
			else {
				SSD_index = not_full_ssd_list.back();
				if ((_SSD_list[SSD_index].storage_usage + _segment_list[index].size) > _SSD_list[SSD_index].storage_space)
					not_full_ssd_list.pop_back();
				else {
					break;
				}
			}
		}

		_segment_list[index].assigned_SSD = SSD_index;
		_SSD_list[SSD_index].assigned_segments.insert(make_pair(_segment_list[index].requested_bandwidth, index));

		_SSD_list[SSD_index].storage_usage += _segment_list[index].size;
		_SSD_list[SSD_index].bandwidth_usage += _segment_list[index].requested_bandwidth;

		//_segment_list[index].is_alloc = true;
	}
}

void update_SSDs_and_insert_new_videos(SSD* _SSD_list, video_segment* _segment_list) {
	vector<double>seg_pop_shuffle(seg_pop, seg_pop + NUM_OF_SEGMENTs);
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(seg_pop_shuffle.begin(), seg_pop_shuffle.end(), g);
	
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].bandwidth_usage = 0;
		_SSD_list[index].assigned_segments.clear();
		_SSD_list[index].ADWD = 0;
	}
	for (int vid = 1; vid <= NUM_OF_SEGMENTs; vid++) {
		int index = vid;
		int SSD_index = _segment_list[index].assigned_SSD;
		if (SSD_index != -1) {
			double bandwidth = seg_pop_shuffle[NUM_OF_SEGMENTs - vid];
			seg_pop_shuffle.pop_back();
			_segment_list[index].requested_bandwidth = bandwidth * total_maximum_bandwidth; //0913 수정
			_SSD_list[SSD_index].bandwidth_usage += _segment_list[index].requested_bandwidth;
			_SSD_list[SSD_index].assigned_segments.insert(make_pair(_segment_list[index].requested_bandwidth, index));
		}
	}
}

double* set_zipf_pop(int length, double alpha, double beta) {
	double* zipf = (double*)malloc(sizeof(double) * length);
	double* pop = (double*)malloc(sizeof(double) * length);
	double sum_caculatedValue = 0;
	double caculatedValue = 0;

	zipf[0] = 0;
	for (int i = 1; i < length + 1; i++) {
		caculatedValue = (double)beta / powl(i, alpha);
		sum_caculatedValue += caculatedValue;
		zipf[i-1] = caculatedValue;
	}
	double sum = 0;
	for (int i = 1; i < length + 1; i++) {
		zipf[i-1] /= sum_caculatedValue;
		pop[i-1] = zipf[i-1];
	}
	return pop;
}