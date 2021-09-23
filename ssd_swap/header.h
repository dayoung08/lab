#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
using namespace std;

#define SEED 111
//#define INFINITY 987654321
#define NUM_OF_SSDs 30 //����Ʈ 20

#define NUM_OF_VIDEOs 2000000 // ����Ʈ 2000000
#define SIZE_OF_VIDEO 10

#define NUM_OF_DATEs 30
#define NUM_OF_TIMEs 4

//#define VIDEO_BANDWIDTH_USAGE 0.5f //������ 4000kbps(�� 4Mbps)��� �����غ��ô�. 4*0.125=0.5,�ϳ��� 0.5MB/s�� ��ϴ�.
#define VIDEO_BANDWIDTH_USAGE 1 //������ 8000kbps(�� 8Mbps)��� �����غ��ô�. 4*0.125=0.5,�ϳ��� 1MB/s�� ��ϴ�.

#define MAX_DWPD 150 //1.50
#define MIN_DWPD 4   //0.04

#define MAX_WAF 40 // 4.0 
#define MIN_WAF 20 // 2.0  //https://www.crucial.com/support/articles-faq-ssd/why-does-SSD-seem-to-be-wearing-prematurely
//https://www.samsung.com/semiconductor/global.semi.static/Multi-stream_Cassandra_Whitepaper_Final-0.pdf �̰� 1~3.2. 1�̸� �׳� ������ �ݷ������� ���� ���� ������ ���°�

#define MAX_SSD_BANDWIDTH 5000
#define MIN_SSD_BANDWIDTH 400

#define OUR_METHOD 1
#define BENCHMARK 2

#define ALPHA 0.729 //0.729 // ���� ������ 0.729��. 1-��Ÿ. �α⵵ - ���� ������ ����ϴ� ���� ��Ÿ ��
#define BETA  1

struct SSD {
	int index;

	double DWPD;
	double ADWD;

	double WAF;

	int storage_space;
	double maximum_bandwidth;

	int storage_usage;
	double bandwidth_usage;

	set<pair<double, int>, less<pair<double, int>>> assigned_VIDEOs;
};

struct video_VIDEO {
	int index;

	int size;
	double requested_bandwidth;

	int assigned_SSD;

	bool is_alloc;
};

void initalization(SSD* _SSD_list, video_VIDEO* _VIDEO_list);
double* set_zipf_pop(int length, double alpha, double beta);
void update_SSDs_and_insert_new_videos(SSD* _SSD_list, video_VIDEO* _VIDEO_list);

int run(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _mothod);
int our_algorithm(SSD* _SSD_list, video_VIDEO* _VIDEO_list);
int benchmark(SSD* _SSD_list, video_VIDEO* _VIDEO_list);

void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs);
pair<double, double> get_slope_to(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid);
pair<double, double> get_slope_from(SSD* _SSD_list, video_VIDEO* _VIDEO_list, int _from_ssd, int _to_ssd, int _from_vid);