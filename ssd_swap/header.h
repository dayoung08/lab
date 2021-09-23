#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
using namespace std;

#define SEED 111
//#define INFINITY 987654321
#define NUM_OF_SSDs 30 //디폴트 20

#define NUM_OF_VIDEOs 2000000 // 디폴트 2000000
#define SIZE_OF_VIDEO 10

#define NUM_OF_DATEs 30
#define NUM_OF_TIMEs 4

//#define VIDEO_BANDWIDTH_USAGE 0.5f //비디오가 4000kbps(즉 4Mbps)라고 가정해봅시다. 4*0.125=0.5,하나에 0.5MB/s가 듭니다.
#define VIDEO_BANDWIDTH_USAGE 1 //비디오가 8000kbps(즉 8Mbps)라고 가정해봅시다. 4*0.125=0.5,하나에 1MB/s가 듭니다.

#define MAX_DWPD 150 //1.50
#define MIN_DWPD 4   //0.04

#define MAX_WAF 40 // 4.0 
#define MIN_WAF 20 // 2.0  //https://www.crucial.com/support/articles-faq-ssd/why-does-SSD-seem-to-be-wearing-prematurely
//https://www.samsung.com/semiconductor/global.semi.static/Multi-stream_Cassandra_Whitepaper_Final-0.pdf 이건 1~3.2. 1이면 그냥 가비지 콜렉션으로 인한 쓰기 증폭이 없는것

#define MAX_SSD_BANDWIDTH 5000
#define MIN_SSD_BANDWIDTH 400

#define OUR_METHOD 1
#define BENCHMARK 2

#define ALPHA 0.729 //0.729 // 보통 비디오는 0.729임. 1-세타. 인기도 - 지프 분포에 사용하는 알파 베타 값
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