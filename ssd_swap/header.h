#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
#include <fstream>   
#include <sstream>
using namespace std;

#define SEED 111
//#define INFINITY 987654321

#define NONE_ALLOC -1

#define PLACEMENT_OURS 1
#define PLACEMENT_BANDWIDTH_AWARE 2
#define PLACEMENT_RANDOM 3
#define MIGRATION_OURS 1
#define MIGRATION_BANDWIDTH_AWARE 2

#define ALPHA 0.729 //0.729 // 보통 비디오는 0.729임. 1-세타. 인기도 - 지프 분포에 사용하는 알파 베타 값
#define BETA  1

#define AVR_ADWD_LIMIT 1
#define NUM_OF_REQUEST_PER_SEC 30000 // 1초에 20000번의 제공 요청이 클라이언트들에게서 온다고 가정.

struct SSD {
	int index;

	double DWPD;
	double WAF;

	double storage_capacity;
	double maximum_bandwidth;

	double storage_usage;
	double bandwidth_usage;
	double daily_write_MB;
	double ADWD;

	double total_write_MB;
	int running_days;

	set<pair<double, int>, less<pair<double, int>>> assigned_VIDEOs_low_bandwidth_first;

	string node_hostname; // for hadoop datanode
};

struct VIDEO_SEGMENT {
	int index;
	string path;  // for hadoop file path

	double size;
	double bandwidth; // once

	double requested_bandwidth;
	double popularity;

	int assigned_SSD;

	bool is_alloc;
};


void simulation();
void testbed_placement();
void testbed_migration();

void initalization_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos); 
void update_new_video_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos);
void initalization_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int& num_of_SSDs, int& num_of_videos);
void update_new_video_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos, int& _num_of_new_videos);

double* set_zipf_pop(int length, double alpha, double beta);
bool is_not_enough_storage_space(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _to_ssd, int _from_vid);
string* split(string str, char Delimiter);

int placement(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _num_of_SSDs, int _num_of_videos);
int placement_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos);
int placement_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos);
int placement_random(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos);
void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _video_index, int _ssd_index);

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _num_of_SSDs);
int migration_myAlgorithm(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs);
int migration_bandwidth_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs);
void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);
void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> element, int from_ssd, int to_ssd, int from_vid);
void update_infomation(SSD* _SSD_list, bool* _is_over_load, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs, int _num_of_SSDs);
pair<pair<double, double>, double> get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
pair<pair<double, double>, double> get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_new_videos);
void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_existed_videos, int* _prev_assigned_SSD);
void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos);