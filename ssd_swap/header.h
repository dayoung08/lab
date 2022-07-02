#include <iostream>
#include <set>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
#include <fstream>   
#include <sstream>
#include <queue>
using namespace std;

#define SEED 1234567
//#define INFINITY 987654321

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
#define MIGRATION_BANDWIDTH_AWARE 8
#define MIGRATION_STORAGE_SPACE_AWARE 9
#define MIGRATION_LIFETIME_AWARE 10
#define MIGRATION_RANDOM 11
#define MIGRATION_ROUND_ROBIN 12

#define ALPHA 0.729 //0.729 // 보통 비디오는 0.729임. 1-세타. 인기도 - 지프 분포에 사용하는 알파 베타 값
#define BETA  1

#define VIDEO_BANDWIDTH 2.5f //MB/s 
#define VIDEO_SIZE 15.0f; //MB

#define MIN_RUNNING_DAY 1
#define MAX_RUNNING_DAY 70
//#define RUNNING_DAY 20
//당연히 이거 1일때가 제일 잘 나옴 으앙....

struct SSD {
	int index;

	double DWPD;

	double storage_capacity; //MB 기준
	double maximum_bandwidth; //MB/s 기준

	double storage_usage;
	double total_bandwidth_usage;
	set<pair<double, int>, less<pair<double, int>>> total_assigned_VIDEOs_low_bandwidth_first;

	double ADWD;
	double total_write;  //MB 기준
	int age; // day 기준, 1에서 시작하므로 Hour / 24 + 1

	string node_hostname; // for hadoop datanode
	string storage_folder_name; // for hadoop storage로 할당한 stoage의 디렉토리 폴더 이름

	~SSD();
};

struct VIDEO_CHUNK {
	int index;
	string path;  // for hadoop file path
	double size;
	double once_bandwidth; // once

	double requested_bandwidth;
	double popularity;

	int assigned_SSD;
};
//initialization.cpp
void SSD_initalization_for_simulation(SSD* _SSD_list, int _num_of_SSDs);
void video_initalization_for_simulation(VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_videos, int _num_of_request_per_sec);
void setting_for_placement_in_simulation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_video, int _num_of_request_per_secs);
void setting_for_migration_in_simulation(SSD* _SSD_list, VIDEO_CHUNK* _existed_VIDEO_CHUNK_list, VIDEO_CHUNK* _new_VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_existing_videos, int _num_of_new_videos, int _num_of_request_per_sec, int _time);

SSD* SSD_initalization_for_testbed(int& _num_of_SSDs);
VIDEO_CHUNK* video_initalization_for_testbed(int& num_of_existing_videos, int& num_of_new_videos, int _num_of_SSDs, int _num_of_request_per_sec, int _migration_method, bool _is_migration);
void setting_for_placement_in_testbed(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int& num_of_SSDs, int& num_of_videos, int _num_of_request_per_sec);
void setting_for_migration_in_testbed(SSD* _SSD_list, VIDEO_CHUNK* VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int& _num_of_existing_videos, int& num_of_new_videos, int _num_of_request_per_sec, int _time);

double* set_zipf_pop(int length, double alpha, double beta);
bool is_full_storage_space(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _to_ssd, int _from_vid);
string* split(string str, char Delimiter);

//algorithm_placement.cpp
int placement(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _method, int _num_of_SSDs, int _num_of_videos);
int placement_resource_aware(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _placement_method, int _num_of_SSDs, int _num_of_videos);
int placement_basic(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _placement_method, int _num_of_SSDs, int _num_of_videos);
void allocate(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _ssd_index, int _video_index);

//algorithm_migration_cpp
int migration(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos);
int migration_of_two_phase(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos, int* _prev_SSD, double* _MB_write);
int migration_benchmark(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_videos, int* _prev_SSD, double* _MB_write);

pair<int, pair<int, int>> determine_migration_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, set<pair<int, pair<double, pair<double, int>>>, greater<pair<int, pair<double, pair<double, int>>>>>* under_load_list, int _from_ssd, int _from_vid);
void set_serviced_video(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, int ssd, bool flag, int* _migration_num, int* _prev_SSD, double* _MB_write);
void swap(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid, int* _migration_num, int* _prev_SSD, double* _MB_write);
void reallocate(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, pair<double, int> _element, int _from_ssd, int _to_ssd, int _from_vid, int* _migration_num, int* _prev_SSD, double* _MB_write);
void update_SSD_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, bool* _is_over_load, bool* _is_full, set<pair<double, int>, greater<pair<double, int>>>* _over_load_SSDs, int _num_of_SSDs);
int get_migration_flag(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _from_ssd, int _to_ssd, int _from_vid, int _to_vid);

//information_file_create.cpp
void create_placement_infomation(SSD* _SSD_list, VIDEO_CHUNK* _new_VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_new_videos);
void create_migration_infomation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_existing_videos, int _num_of_new_videos, int* _prev_assigned_SSD);
void create_SSD_and_video_list(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, bool _is_migration);

//main.cpp
void placement(bool _is_simulation);
void migartion_in_simulation();
void migration_in_testbed(int _time);