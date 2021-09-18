#include "header.h"


double total_maximum_bandwidth = 0;
double* vid_pop;
int cnt = 0;
void initalization(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].index = index;
		_SSD_list[index].DWPD = ((double)(rand() % (MAX_DWPD - MIN_DWPD + 1) + MIN_DWPD)) / 100;
		_SSD_list[index].WAF = ((double)(rand() % (MAX_WAF - MIN_WAF + 1) + MIN_WAF)) / 10;
		_SSD_list[index].DWPD /= _SSD_list[index].WAF;
		_SSD_list[index].storage_space = 500000 * pow(2, rand() % 4); // 0.5, 1, 2, 4TB
		//_SSD_list[index].storage_space = 2000000 * 0.9095;  //보통 2테라면 약간 더 낮아져서

		_SSD_list[index].maximum_bandwidth = rand() % (MAX_SSD_BANDWIDTH - MIN_SSD_BANDWIDTH + 1) + MIN_SSD_BANDWIDTH;
		total_maximum_bandwidth += _SSD_list[index].maximum_bandwidth; 
		//https://tekie.com/blog/hardware/ssd-vs-hdd-speed-lifespan-and-reliability/
		//https://www.quora.com/What-is-the-average-read-write-speed-of-an-SSD-hard-drive

		_SSD_list[index].ADWD = 0;
		_SSD_list[index].storage_usage = 0;
		_SSD_list[index].bandwidth_usage = 0;
	}

	double cal = (VIDEO_BANDWIDTH_USAGE * NUM_OF_VIDEOs);
	if (cal > total_maximum_bandwidth) {
		printf("세그먼트 숫자의 밴드윗 총 합이 SSD 밴드윗 총 합보다 큼\n");
		exit(0);
	}

	vid_pop = set_zipf_pop(NUM_OF_VIDEOs, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + cnt);
	cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);

	vector<int> not_full_ssd_list(NUM_OF_SSDs);
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		not_full_ssd_list[ssd - 1] = ssd;
	}
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int index = vid;
		_VIDEO_list[index].index = index;
		_VIDEO_list[index].size = SIZE_OF_VIDEO;

		double pop = vid_pop_shuffle[NUM_OF_VIDEOs - vid];
		vid_pop_shuffle.pop_back();
		_VIDEO_list[index].requested_bandwidth = pop * (VIDEO_BANDWIDTH_USAGE * NUM_OF_VIDEOs); //0916 수정
		
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
				if ((_SSD_list[SSD_index].storage_usage + _VIDEO_list[index].size) > _SSD_list[SSD_index].storage_space)
					not_full_ssd_list.pop_back();
				else {
					break;
				}
			}
		}

		_VIDEO_list[index].assigned_SSD = SSD_index;
		_SSD_list[SSD_index].assigned_VIDEOs.insert(make_pair(_VIDEO_list[index].requested_bandwidth, index));

		_SSD_list[SSD_index].storage_usage += _VIDEO_list[index].size;
		_SSD_list[SSD_index].bandwidth_usage += _VIDEO_list[index].requested_bandwidth;

		_VIDEO_list[index].is_alloc = true;
	}
}

void update_SSDs_and_insert_new_videos(SSD* _SSD_list, video_VIDEO* _VIDEO_list) {
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + cnt);
	cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].bandwidth_usage = 0;
		_SSD_list[index].assigned_VIDEOs.clear();
	}
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int index = vid;
		int SSD_index = _VIDEO_list[index].assigned_SSD;
		if (SSD_index != -1) {
			double pop = vid_pop_shuffle[NUM_OF_VIDEOs - vid];
			vid_pop_shuffle.pop_back();
			_VIDEO_list[index].requested_bandwidth = pop * (VIDEO_BANDWIDTH_USAGE * NUM_OF_VIDEOs); //0916 수정
			_SSD_list[SSD_index].bandwidth_usage += _VIDEO_list[index].requested_bandwidth;
			_SSD_list[SSD_index].assigned_VIDEOs.insert(make_pair(_VIDEO_list[index].requested_bandwidth, index));
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