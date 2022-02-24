#include <iostream>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <set>
#include <ctime>
#include <random> 
#include <vector> 

using namespace std;

#define SEED 1111
#define INF 987654321

//#define PERIOD 30 // 한 달에 한 번 돈 낸다고 가정하자
#define LINEAR_MODEL 0
#define ONOFF_MODEL 1
//#define STEP_MODEL 2

//아래는 교수님이 주신 파일 기반. 코드 보니까 클라이언트 수 x 유저 수를 채널로 해도 될 것 같다. 필요 시 coord.c 파일 수정해서 파일들 다시 뽑으면 될 듯.
//#define NUM_OF_CLIENT	816
//#define NUM_OF_SERVER	125
//#define NUM_OF_USER	10 // the number of user for each ip block
//#define ES_NUM 125 // 교수님 주신 파일의 NUM_OF_SERVER
#define NUM_OF_ES 100 // 50 75 ((100)) 125 150
#define NUM_OF_MACHINE 5

//#define CHANNEL_NUM 8160  // 교수님 주신 파일의 NUM_OF_CLIENT * NUM_OF_USER
#define NUM_OF_CHANNEL 6000 //2000~10000 사이이고, ((6000이 기준))

//#define VERSION_NUM 7
//#define VERSION_SET_NUM 32 // 오리지널 버전 제외하고, 마지막 버전은 반드시 저장. 2^(7-2) set 1이 오리지널과 마지막 버전만 들어있는 것.
//set 1 = (1)00000(1)이라서 1번, 오리지널 버전만
//set 2 = (1)00001(1)이라서 1번, 오리지널 버전과 함께 2번 버전만,
//set 3 = (1)00010(1)이라서 1번, 오리지널 버전과 함께 3번 버전만.

#define HVP 1
#define MVP 2
#define LVP 3
#define RVP 4

#define VMAF 0
#define SSIM 1
#define PSNR 2
#define MOS 3

//#define ALPHA 2.0 // 인기도 - 지프 분포에 사용하는 알파 베타 값
//#define BETA 1.0

#define SIGMA 1 // 버전 인기도 - 노멀 분포에 사용하는 값
 //0.25 0.5 0.75 <Default 1> 1.25 1.5 1.75 2 사이의 값.

#define K_gamma 0.399 // 인기도 - 감마 분포에 사용하는 k, 세타값
#define THETA_gamma 14260.0

#define M_E 2.7182818284590452354 /* e */
#define PI 3.1415926535897932384 /* pi */

#define GHz_WF_AP 1
#define GHz_WF_HPF 2
#define cost_WF_AP 3
#define cost_WF_HPF 4
#define LPF_AP 5
#define LPF_HPF 6
#define RD_AP 7
#define RD_HPF 8
#define Mbps_WF_AP 9
#define Mbps_WF_HPF 10

struct location {
	double latitude;
	double longitude;
};

class server {
public:
	int index;
	double processing_capacity;
	double maximum_bandwidth;
	location server_location; // int capacity; // server's capacity

	int coverage; // 커버리지

	//int machine_type; // 엣지 기기의 종류 
	int cost_model_type; // 비용 함수의 종류
	double cpu_usage_cost_alpha; // 논문에서 alpha 값, CPU usage가 100%일때의 값임.
	double bandwidth_cost_alpha; // 논문에서 beta 값, bandwidth가 100%일때의 값임.

	//아래는 채널 할당으로 인해 갱신되는 값.
	/*int total_GHz;
	double total_transfer_data_size;
	double total_cost;*/
};

class channel {
public:
	location broadcaster_location; // 이 채널 broadcaster의 위치. 이것과 엣지의 distance를 구해야함.

	int index; // channel;
	int version_pop_type; //HVP MVP LVP RVP

	double* video_quality;//[VERSION_NUM + 1];
	double* popularity;//[VERSION_NUM + 1]; // [0] 여기는 전체 채널의 pop, 즉 전체 버전 version의 pop 합.
	double* video_GHz;//[VERSION_NUM + 1];
	double* video_Mbps;//[VERSION_NUM + 1];
	double* pwq;//[VERSION_NUM + 1]; // weighted video quality라고 쓰여있음. 하도 평소에 video pwq이라 불러서 코딩때도 이렇게 함.
	
	double* sum_of_video_quality;//[VERSION_SET_NUM + 1];
	double* sum_of_pwq;//[VERSION_SET_NUM + 1];
	//이 set 지우지 말것. pwq 합 계산할때 이걸로 돌리는게 제일 편하다.
	double* sum_of_version_set_GHz;
	double* sum_of_version_set_Mbps;

	double get_channel_popularity();
	bool available_server_list[NUM_OF_ES]; // true: user i is in server j's coverage

	//아래는 알고리즘의 결과로 결정될 파라미터
	int allocated_server_index; //이게 중요함. 교수님이 주신 파일에서 int assign[NUM_OF_CLIENT * NUM_OF_USER]; // assigned server index -> 이것과 같은 역할임.
	int determined_version_set; //이게 중요함 2.
};

class bitrate_version_set {
public: //그냥 전부 public 가자
	int index; // 0은 우리가 쓰는 zencoder 조합.

	int version_num;
	int version_set_num;

	int* resolution; //1080p면 값이 1080임
	int* bitrate; // 논문에서의 r을 위한 값. 단 여기선 kbps고, r은 Mbps(GHz 구할 때 변환함). set_GHz 최상단의 논문의 수식 참고.
	//double* data_size; 
	// 엣지에서 데이터 외부 전송에 대한 돈을 받기 때문에...  bitrate(kbps) -> MB/s로 변환함. 
	// GHz도 초당 사용량이니 이쪽도 초당 데이터 전송량으로 하면 되니까. 그러므로 이 값이 데이터 전송량이 되는 것.
	double* mean;

	int number_for_bit_opration;
	int set_versions_number_for_bit_opration;

	bitrate_version_set(int _index, int _metric_type); //initiation.cpp에 구현 있음
};

/* channel.cpp */
void channel_initialization(channel* _channel_list, bitrate_version_set* _version_set, int _version_pop_type, int _metric_type);
void set_video_metric(channel* _channel, bitrate_version_set* _version_set, int _metric_type);
void set_GHz(channel* _channel, bitrate_version_set* _version_set);
void set_PWQ(channel* _channel, bitrate_version_set* _version_set);
double* set_gamma_pop(int length, double k, double theta);
double* set_version_pop(bitrate_version_set* _bitrate_version_set, int _version_pop_type);

/* server.cpp */
void server_initalization(server* _server_list, int _model, bool _bandwidth_model_flag);
double calculate_ES_cpu_usage_cost(server* _server, double _used_GHz, int _model);
double calculate_ES_bandwidth_cost(server* _server, double _used_Mbps, int _model);
double get_total_charge(server* _server_list, int _cost_model);
void set_coverage_infomation(channel* _channel_list, server* _server_list);
double calculate_distance(channel* _channel, server* _server);
double deg2rad(double _deg);
double rad2deg(double _rad);

/* bitrate_version_set.cpp */
void set_version_set(bitrate_version_set* _version_set, short* _selected_set, short** _selected_ES);
void is_not_success_for_lowest_allocation(short** _selected_ES, int* _ES_count, bool is_not_satisfied_cost_constraints);

/* algorithm.cpp */
void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model);
//void TD_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _GHz_limit, short* _selected_set);
void TDA_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool _is_lowest_only_mode);
void CR_phase(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _total_cost, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model, bool* _turn_on_at_lowest);

/* comparison_schemes*/
void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, int _model);
void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);

void GHz_worst_fit_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void GHz_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void cost_worst_fit_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void cost_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void lowest_price_first_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void lowest_price_first_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void random_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void random_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void Mbps_worst_fit_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
void Mbps_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, double _cost_limit, short* _selected_set, short** _selected_ES, double* _used_GHz, double* _used_Mbps, int* _ES_count, int _model);
