#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
using namespace std;

#define SEED 1234
//#define INFINITY 987654321
#define NUM_OF_SSDs 30

#define NUM_OF_SEGMENTs 40000 //������ �� �� �۾����� ���׸�Ʈ �ϳ��� ������� �ʹ� Ŀ���� ������
#define SIZE_OF_SEGMENT 1 // Z����

#define NUM_OF_DATEs 10
#define NUM_OF_TIMEs 4

//#define MAX_VIDEO_BANDWIDTH_USAGE 5000
//#define MIN_VIDEO_BANDWIDTH_USAGE 200

#define MAX_DWPD 150 // 1.5
#define MIN_DWPD 4   //0.04

#define MAX_SSD_BANDWIDTH 5000
#define MIN_SSD_BANDWIDTH 400

#define OUR_METHOD 1
#define COMPARATIVE_METHOD 2

#define ALPHA 0.2 // ���� ������ 0.729��. �α⵵ - ���� ������ ����ϴ� ���� ��Ÿ ��
#define BETA  1.0

struct SSD {
	int index;

	double DWPD;
	double ADWD;

	double MB_write;
	int storage_space;
	double maximum_bandwidth;

	int storage_usage;
	double bandwidth_usage;

	set<pair<double, int>, less<pair<double, int>>> assigned_segments;
};

struct video_segment {
	int index;

	int size;
	double requested_bandwidth;

	int assigned_SSD;

	bool is_alloc;
};

void initalization(SSD* _SSD_list, video_segment* _segment_list);
double* set_zipf_pop(int length, double alpha, double beta);
void update_SSDs_and_insert_new_videos(SSD* _SSD_list, video_segment* _segment_list);

int run(SSD* _SSD_list, video_segment* _segment_list, int _mothod);
int our_algorithm(SSD* _SSD_list, video_segment* _segment_list);
int benchmark(SSD* _SSD_list, video_segment* _segment_list);

void update_infomation(SSD* _SSD_list, bool* _is_over_load, bool* _is_full, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs);
pair<double, double> get_slope_to(SSD* _SSD_list, video_segment* _segment_list, int _from_ssd, int _to_ssd, int _from_seg, bool* _is_full);
pair<double, double> get_slope_from(SSD* _SSD_list, video_segment* _segment_list, int _from_ssd, int _to_ssd, int _from_seg, bool* _is_full);