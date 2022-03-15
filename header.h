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

#define SSD_TYPE 10

#define NONE_ALLOC -1
#define VIRTUAL_SSD 0

//#define PLACEMENT_OURS 0
#define PLACEMENT_OURS 1
#define PLACEMENT_BANDWIDTH_AWARE 2
#define PLACEMENT_STORAGE_SPACE_AWARE 3
#define PLACEMENT_LIFETIME_AWARE 4
#define PLACEMENT_RANDOM 5
#define PLACEMENT_ROUND_ROBIN 6

#define MIGRATION_OURS 7
#define MIGRATION_THROUGHPUT_AWARE 8
#define MIGRATION_BANDWIDTH_AWARE 9
#define MIGRATION_STORAGE_SPACE_AWARE 10
#define MIGRATION_LIFETIME_AWARE 11

#define ALPHA 0.729 //0.729 // ���� ������ 0.729��. 1-��Ÿ. �α⵵ - ���� ������ ����ϴ� ���� ��Ÿ ��
#define BETA  1

#define VIDEO_BANDWIDTH 1.5f //������ 12000kbps(�� 12Mbps)��� �����غ��ô�.12*0.125=1.25,�ϳ��� 1.25MB/s�� ��ϴ�.
#define VIDEO_SIZE 15.0f; // 1.5MB/s x 10�� 15MB

struct SSD {
	int index;

	double DWPD;

	double storage_capacity;
	double maximum_bandwidth;

	double storage_usage;
	double total_bandwidth_usage;
	set<pair<double, int>, less<pair<double, int>>> total_assigned_VIDEOs_low_bandwidth_first;

	double ADWD;
	double total_write_MB;
	int running_days;

	double serviced_bandwidth_usage;

	string node_hostname; // for hadoop datanode
};

struct VIDEO_SEGMENT {
	int index;
	string path;  // for hadoop file path
	double size;
	double once_bandwidth; // once

	double requested_bandwidth;
	double popularity;

	int assigned_SSD;
	bool is_serviced;

	int type; // � ������ ���׸�Ʈ�ΰ�? 
};


void simulation_placement();
void simulation_migartion();
void testbed_placement();
void testbed_migration(bool _has_new_files);

void placed_video_init_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_video, int _num_of_request_per_secs);
void SSD_initalization_for_simulation(SSD* _SSD_list, int _num_of_SSDs);
void video_initalization_for_simulation(VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_videos, int _num_of_request_per_sec);
void migrated_video_init_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos, int _num_of_request_per_sec, int _day);

void placed_video_init_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos, int _num_of_request_per_sec);
void SSD_initalization_for_testbed(SSD* _SSD_list, int& _num_of_SSDs);
void video_initalization_for_testbed(VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int& num_of_videos, int _num_of_request_per_sec, bool is_new);
void migrated_video_init_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _migration_method, int& _num_of_SSDs, int& _num_of_existed_videos, int& _num_of_new_videos, int _num_of_request_per_sec, bool _has_new_files);

double* set_zipf_pop(int length, double alpha, double beta);
bool is_full_storage_space(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _to_ssd, int _from_vid);
void set_serviced_video(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos);
string* split(string str, char Delimiter);

int placement(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _num_of_SSDs, int _num_of_videos);
int placement_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _placement_method, int _num_of_SSDs, int _num_of_videos);
int placement_basic(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _placement_method, int _num_of_SSDs, int _num_of_videos);
void allocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _ssd_index, int _video_index);

int migration(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos);
int migration_resource_aware(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_videos);
void swap(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);
void reallocate(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, pair<double, int> element, int from_ssd, int to_ssd, int from_vid);
void update_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, bool* _is_over_load, bool* _is_imposible, set<pair<double, int>, greater<pair<double, int>>>* _bandwidth_usage_of_SSDs, int _num_of_SSDs, vector<pair<double, int>>* _eliminated_video_list, bool* _is_inserted_eliminated_video_list);
double get_slope_to(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
double get_slope_from(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _from_ssd, int _to_ssd, int _from_vid);
int get_migration_flag(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);

void create_placement_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_new_videos);
void create_migration_infomation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _migration_method, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos, int* _prev_assigned_SSD);
void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos);