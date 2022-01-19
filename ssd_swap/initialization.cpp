#include "header.h"


double total_maximum_bandwidth = 0;
double* vid_pop;
int rand_cnt = 0;
void initalization(SSD* _SSD_list, VIDEO* _VIDEO_list) {
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

	/*double cal = (VIDEO_BANDWIDTH_USAGE * NUM_OF_VIDEOs);
	if (cal > total_maximum_bandwidth) {
		printf("세그먼트 숫자의 밴드윗 총 합이 SSD 밴드윗 총 합보다 큼\n");
		exit(0);
	}*/
	total_maximum_bandwidth *= 0.7;
	vid_pop = set_zipf_pop(NUM_OF_VIDEOs, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + rand_cnt);
	rand_cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);

	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int index = vid;
		_VIDEO_list[index].index = index;
		_VIDEO_list[index].size = SIZE_OF_VIDEO;
		_VIDEO_list[index].is_alloc = false;
		_VIDEO_list[index].assigned_SSD = 0;

		double pop = vid_pop_shuffle[NUM_OF_VIDEOs - vid]; // 혹은 [vid-1]
		vid_pop_shuffle.pop_back();
		_VIDEO_list[index].requested_bandwidth = pop * total_maximum_bandwidth; //0916 수정

		//여기에 원래 placement가 들어갔는데, 이 부분을 지금은 뺐다.
	}
}

void update_video_bandwidth(SSD* _SSD_list, VIDEO* _VIDEO_list) {
	//원래 새로운 비디오를 추가하는 것도 넣으려고 했으나 지금은 아님.
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + rand_cnt);
	rand_cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	
	for (int ssd = 1; ssd <= NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].bandwidth_usage = 0;
		_SSD_list[index].assigned_VIDEOs_low_bandwidth_first.clear();
	}
	for (int vid = 1; vid <= NUM_OF_VIDEOs; vid++) {
		int index = vid;
		int SSD_index = _VIDEO_list[index].assigned_SSD;
		if (SSD_index != -1) {
			double pop = vid_pop_shuffle[NUM_OF_VIDEOs - vid]; // pop을 바꿔줌으로써 비디오의 밴드윗들을 다 바꿔줬음.
			vid_pop_shuffle.pop_back();
			_VIDEO_list[index].requested_bandwidth = pop * total_maximum_bandwidth; //0916 수정
			_SSD_list[SSD_index].bandwidth_usage += _VIDEO_list[index].requested_bandwidth;
			_SSD_list[SSD_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_list[index].requested_bandwidth, index));
			//원래 할당되었던 곳에 그대로 넣음. 계산 쉽게 하려고 다 뺐던거라...
		}
	}
}

double* set_zipf_pop(int length, double alpha, double beta) {
	double* zipf = new double[length];
	double* pop = new double[length];
	double sum_caculatedValue = 0;
	double caculatedValue = 0;

	zipf[0] = 0;
	for (int i = 1; i <= length; i++) {
		caculatedValue = (double)beta / powl(i, alpha);
		sum_caculatedValue += caculatedValue;
		zipf[i-1] = caculatedValue;
	}
	double sum = 0;
	for (int i = 1; i <= length; i++) {
		zipf[i-1] /= sum_caculatedValue;
		pop[i-1] = zipf[i-1];
	}
	delete[] zipf;
	return pop;
}

bool is_not_enough_storage_space(SSD* _SSD_list, VIDEO* _VIDEO_list, int _to_ssd, int _from_vid) {
	return (_SSD_list[_to_ssd].storage_usage + _VIDEO_list[_from_vid].size) > _SSD_list[_to_ssd].storage_space;
}
