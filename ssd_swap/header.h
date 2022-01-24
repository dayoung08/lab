#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
using namespace std;

#define SEED 111
//#define INFINITY 987654321
#define NUM_OF_SSDs 20 //디폴트 20

#define NUM_OF_VIDEOs 2000000 // 디폴트 2000000
#define SIZE_OF_VIDEO 10

#define NUM_OF_DATEs 1
#define NUM_OF_TIMEs 1

//#define VIDEO_BANDWIDTH_USAGE 0.5f //비디오가 4000kbps(즉 4Mbps)라고 가정해봅시다. 4*0.125=0.5,하나에 0.5MB/s가 듭니다.
#define VIDEO_BANDWIDTH_USAGE 1 //비디오가 8000kbps(즉 8Mbps)라고 가정해봅시다. 4*0.125=0.5,하나에 1MB/s가 듭니다.

#define MAX_DWPD 150 //1.50
#define MIN_DWPD 4   //0.04

#define MAX_WAF 40 // 4.0 
#define MIN_WAF 20 // 2.0  //https://www.crucial.com/support/articles-faq-ssd/why-does-SSD-seem-to-be-wearing-prematurely
//https://www.samsung.com/semiconductor/global.semi.static/Multi-stream_Cassandra_Whitepaper_Final-0.pdf 이건 1~3.2. 1이면 그냥 가비지 콜렉션으로 인한 쓰기 증폭이 없는것

#define MAX_SSD_BANDWIDTH 5000
#define MIN_SSD_BANDWIDTH 400

#define NONE_ALLOC -1

#define PLACEMENT_OURS 1
#define PLACEMENT_BANDWIDTH_AWARE 2
#define PLACEMENT_RANDOM 3
#define MIGRATION_OURS 1
#define MIGRATION_BANDWIDTH_AWARE 2

#define ALPHA 0.729 //0.729 // 보통 비디오는 0.729임. 1-세타. 인기도 - 지프 분포에 사용하는 알파 베타 값
#define BETA  1

struct SSD {
	int index;

	double DWPD;
	double WAF;

	int storage_capacity;
	double maximum_bandwidth;

	int storage_usage;
	double bandwidth_usage;
	double write_MB;
	double ADWD;

	set<pair<double, int>, less<pair<double, int>>> assigned_VIDEOs_low_bandwidth_first;

	string node_hostname; // for hadoop datanode
};

struct VIDEO_SEGMENT {
	int index;

	int size;
	double requested_bandwidth;

	int assigned_SSD;

	bool is_alloc;

	string name;  // for hadoop file path
};

void initalization(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
void update_video_bandwidth(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
double* set_zipf_pop(int length, double alpha, double beta);
bool is_not_enough_storage_space(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _to_ssd, int _from_vid);

int placement(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method);
int placement_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
int placement_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
int placement_random(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _video_index, int _SSD_index);

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _mothod);
int migration_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
int migration_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);
void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> element, int from_ssd, int to_ssd, int from_vid);
void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs);
pair<double, double> get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
pair<double, double> get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list);
void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int* _prev_assigned_SSD);