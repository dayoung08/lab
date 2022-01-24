#include "header.h"


double total_maximum_bandwidth = 0;
double* vid_pop;
int cnt = 0;
void initalization(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list) {
	for (int ssd = 0; ssd < NUM_OF_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].index = ssd_index;
		_SSD_list[ssd_index].storage_capacity = 500000 * pow(2, rand() % 4); // 0.5, 1, 2, 4TB
		//_SSD_list[index].storage_space = 2000000 * 0.9095;  //���� 2�׶�� �ణ �� ��������

		_SSD_list[ssd_index].DWPD = ((double)(rand() % (MAX_DWPD - MIN_DWPD + 1) + MIN_DWPD)) / 100;
		_SSD_list[ssd_index].WAF = ((double)(rand() % (MAX_WAF - MIN_WAF + 1) + MIN_WAF)) / 10;
		_SSD_list[ssd_index].DWPD /= _SSD_list[ssd_index].WAF;
		_SSD_list[ssd_index].maximum_bandwidth = rand() % (MAX_SSD_BANDWIDTH - MIN_SSD_BANDWIDTH + 1) + MIN_SSD_BANDWIDTH;

		total_maximum_bandwidth += _SSD_list[ssd_index].maximum_bandwidth;
		//https://tekie.com/blog/hardware/ssd-vs-hdd-speed-lifespan-and-reliability/
		//https://www.quora.com/What-is-the-average-read-write-speed-of-an-SSD-hard-drive

		_SSD_list[ssd_index].ADWD = 0;
		_SSD_list[ssd_index].storage_usage = 0;
		_SSD_list[ssd_index].bandwidth_usage = 0;

		_SSD_list[ssd_index].node_hostname = "datanode" + to_string(ssd+1);
	}

	/*double cal = (VIDEO_BANDWIDTH_USAGE * NUM_OF_VIDEOs);
	if (cal > total_maximum_bandwidth) {
		printf("���׸�Ʈ ������ ����� �� ���� SSD ����� �� �պ��� ŭ\n");
		exit(0);
	}*/
	total_maximum_bandwidth *= 0.7;
	vid_pop = set_zipf_pop(NUM_OF_VIDEOs, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + cnt);
	cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int video_index = vid;
		_VIDEO_SEGMENT_list[video_index].index = video_index;
		_VIDEO_SEGMENT_list[video_index].size = SIZE_OF_VIDEO;

		double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();
		_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * total_maximum_bandwidth; //0916 ����

		_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
		_VIDEO_SEGMENT_list[video_index].is_alloc = false;

		_VIDEO_SEGMENT_list[video_index].name = "/segment_" + to_string(video_index) + ".mp4";
		//������� �Ҵ�
	}
}

void update_video_bandwidth(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list) {
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + NUM_OF_VIDEOs);
	std::mt19937 g(SEED + cnt);
	cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);

	for (int ssd = 0; ssd < NUM_OF_SSDs; ssd++) {
		int index = ssd;
		_SSD_list[index].bandwidth_usage = 0;
		_SSD_list[index].assigned_VIDEOs_low_bandwidth_first.clear();
	}
	for (int vid = 0; vid < NUM_OF_VIDEOs; vid++) {
		int index = vid;
		int SSD_index = _VIDEO_SEGMENT_list[index].assigned_SSD;
		if (SSD_index != NONE_ALLOC) {
			double pop = vid_pop_shuffle[NUM_OF_VIDEOs - vid];
			vid_pop_shuffle.pop_back();
			_VIDEO_SEGMENT_list[index].requested_bandwidth = pop * total_maximum_bandwidth; //0916 ����
			_SSD_list[SSD_index].bandwidth_usage += _VIDEO_SEGMENT_list[index].requested_bandwidth;
			_SSD_list[SSD_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_VIDEO_SEGMENT_list[index].requested_bandwidth, index));
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
		zipf[i - 1] = caculatedValue;
	}
	double sum = 0;
	for (int i = 1; i < length + 1; i++) {
		zipf[i - 1] /= sum_caculatedValue;
		pop[i - 1] = zipf[i - 1];
	}
	return pop;
}

bool is_not_enough_storage_space(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _to_ssd, int _from_vid) {
	return (_SSD_list[_to_ssd].storage_usage + _VIDEO_SEGMENT_list[_from_vid].size) > _SSD_list[_to_ssd].storage_capacity;
}
