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

//#define PERIOD 30 // �� �޿� �� �� �� ���ٰ� ��������
#define LINEAR_MODEL 0
#define ONOFF_MODEL 1
//#define STEP_MODEL 2

//�Ʒ��� �������� �ֽ� ���� ���. �ڵ� ���ϱ� Ŭ���̾�Ʈ �� x ���� ���� ä�η� �ص� �� �� ����. �ʿ� �� coord.c ���� �����ؼ� ���ϵ� �ٽ� ������ �� ��.
//#define NUM_OF_CLIENT	816
//#define NUM_OF_SERVER	125
//#define NUM_OF_USER	10 // the number of user for each ip block
//#define ES_NUM 125 // ������ �ֽ� ������ NUM_OF_SERVER
#define NUM_OF_ES 100 // 50 75 ((100)) 125 150
#define NUM_OF_MACHINE 5

//#define CHANNEL_NUM 8160  // ������ �ֽ� ������ NUM_OF_CLIENT * NUM_OF_USER
#define NUM_OF_CHANNEL 6000 //2000~10000 �����̰�, ((6000�� ����))

//#define VERSION_NUM 7
//#define VERSION_SET_NUM 32 // �������� ���� �����ϰ�, ������ ������ �ݵ�� ����. 2^(7-2) set 1�� �������ΰ� ������ ������ ����ִ� ��.
//set 1 = (1)00000(1)�̶� 1��, �������� ������
//set 2 = (1)00001(1)�̶� 1��, �������� ������ �Բ� 2�� ������,
//set 3 = (1)00010(1)�̶� 1��, �������� ������ �Բ� 3�� ������.

#define HVP 1
#define MVP 2
#define LVP 3
#define RVP 4

#define VMAF 0
#define SSIM 1
#define PSNR 2
#define MOS 3

//#define ALPHA 2.0 // �α⵵ - ���� ������ ����ϴ� ���� ��Ÿ ��
//#define BETA 1.0

#define SIGMA 1 // ���� �α⵵ - ��� ������ ����ϴ� ��
 //0.25 0.5 0.75 <Default 1> 1.25 1.5 1.75 2 ������ ��.

#define K_gamma 0.399 // �α⵵ - ���� ������ ����ϴ� k, ��Ÿ��
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

	int coverage; // Ŀ������

	//int machine_type; // ���� ����� ���� 
	int cost_model_type; // ��� �Լ��� ����
	double cpu_usage_cost_alpha; // ������ alpha ��, CPU usage�� 100%�϶��� ����.
	double bandwidth_cost_alpha; // ������ beta ��, bandwidth�� 100%�϶��� ����.

	//�Ʒ��� ä�� �Ҵ����� ���� ���ŵǴ� ��.
	/*int total_GHz;
	double total_transfer_data_size;
	double total_cost;*/
};

class channel {
public:
	location broadcaster_location; // �� ä�� broadcaster�� ��ġ. �̰Ͱ� ������ distance�� ���ؾ���.

	int index; // channel;
	int version_pop_type; //HVP MVP LVP RVP

	double* video_quality;//[VERSION_NUM + 1];
	double* popularity;//[VERSION_NUM + 1]; // [0] ����� ��ü ä���� pop, �� ��ü ���� version�� pop ��.
	double* video_GHz;//[VERSION_NUM + 1];
	double* video_Mbps;//[VERSION_NUM + 1];
	double* pwq;//[VERSION_NUM + 1]; // weighted video quality��� ��������. �ϵ� ��ҿ� video pwq�̶� �ҷ��� �ڵ����� �̷��� ��.
	
	double* sum_of_video_quality;//[VERSION_SET_NUM + 1];
	double* sum_of_pwq;//[VERSION_SET_NUM + 1];
	//�� set ������ ����. pwq �� ����Ҷ� �̰ɷ� �����°� ���� ���ϴ�.
	double* sum_of_version_set_GHz;
	double* sum_of_version_set_Mbps;

	double get_channel_popularity();
	bool available_server_list[NUM_OF_ES]; // true: user i is in server j's coverage

	//�Ʒ��� �˰����� ����� ������ �Ķ����
	int allocated_server_index; //�̰� �߿���. �������� �ֽ� ���Ͽ��� int assign[NUM_OF_CLIENT * NUM_OF_USER]; // assigned server index -> �̰Ͱ� ���� ������.
	int determined_version_set; //�̰� �߿��� 2.
};

class bitrate_version_set {
public: //�׳� ���� public ����
	int index; // 0�� �츮�� ���� zencoder ����.

	int version_num;
	int version_set_num;

	int* resolution; //1080p�� ���� 1080��
	int* bitrate; // �������� r�� ���� ��. �� ���⼱ kbps��, r�� Mbps(GHz ���� �� ��ȯ��). set_GHz �ֻ���� ���� ���� ����.
	//double* data_size; 
	// �������� ������ �ܺ� ���ۿ� ���� ���� �ޱ� ������...  bitrate(kbps) -> MB/s�� ��ȯ��. 
	// GHz�� �ʴ� ��뷮�̴� ���ʵ� �ʴ� ������ ���۷����� �ϸ� �Ǵϱ�. �׷��Ƿ� �� ���� ������ ���۷��� �Ǵ� ��.
	double* mean;

	int number_for_bit_opration;
	int set_versions_number_for_bit_opration;

	bitrate_version_set(int _index, int _metric_type); //initiation.cpp�� ���� ����
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
