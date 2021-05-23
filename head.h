#include <iostream>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <set>
#include <cmath>
#include <ctime>
#include <random> 

using namespace std;

#define SEED 1234
#define INF 987654321

#define PERIOD 30 // �� �޿� �� �� �� ���ٰ� ��������

//�Ʒ��� �������� �ֽ� ���� ���. �ڵ� ���ϱ� Ŭ���̾�Ʈ �� x ���� ���� ä�η� �ص� �� �� ����. �ʿ� �� coord.c ���� �����ؼ� ���ϵ� �ٽ� ������ �� ��.
//#define NUM_OF_CLIENT	816
//#define NUM_OF_SERVER	125
//#define NUM_OF_USER	10 // the number of user for each ip block
//#define ES_NUM 125 // ������ �ֽ� ������ NUM_OF_SERVER�� �ϴ� �ʹ� ������ ^^;;;
#define ES_NUM 500

#define AWS_NUM 2
#define MS_AZURE_NUM 3
#define ES_TYPE_NUM (AWS_NUM+MS_AZURE_NUM)
#define CHANNEL_NUM 8160  // ������ �ֽ� ������ NUM_OF_CLIENT * NUM_OF_USER

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

#define RR_HPF 1
#define RR_AP 2
#define RA_HPF 3
#define RA_AP 4
#define PA_HPF 5
#define PA_AP 6

struct location {
	double latitude;
	double longitude;
};

class server {
public:
	int index;
	int processing_capacity;
	location server_location; // int capacity; // server's capacity

	int coverage; // Ŀ������
	double cost_coefficient_for_GHz;
	double cost_coefficient_for_data_size;

	//�Ʒ��� ä�� �Ҵ����� ���� ���ŵǴ� ��.
	int total_GHz;
	double total_transfer_data_size;
	double total_cost;
};

class channel {
public:
	location broadcaster_location; // �� ä�� broadcaster�� ��ġ. �̰Ͱ� ������ distance�� ���ؾ���.

	int index; // channel;
	int version_pop_type; //HVP MVP LVP RVP

	double* video_quality;//[VERSION_NUM + 1];
	double* popularity;//[VERSION_NUM + 1]; // [0] ����� ��ü ä���� pop, �� ��ü ���� version�� pop ��.
	double* video_GHz;//[VERSION_NUM + 1];
	double* pwq;//[VERSION_NUM + 1]; // weighted video quality��� ��������. �ϵ� ��ҿ� video pwq�̶� �ҷ��� �ڵ����� �̷��� ��.

	double* sum_of_video_quality;//[VERSION_SET_NUM + 1];
	double* sum_of_pwq;//[VERSION_SET_NUM + 1];
	double* sum_of_transfer_data_size;//[VERSION_SET_NUM + 1];
	//�� set ������ ����. pwq �� ����Ҷ� �̰ɷ� �����°� ���� ���ϴ�.
	double* sum_of_version_set_GHz;

	double get_channel_popularity();
	bool available_server_list[ES_NUM]; // true: user i is in server j's coverage

	//�Ʒ��� �˰������� ����� ������ �Ķ����
	int allocated_server_index; //�̰� �߿���. �������� �ֽ� ���Ͽ��� int assign[NUM_OF_CLIENT * NUM_OF_USER]; // assigned server index -> �̰Ͱ� ���� ������.
	int determined_version_set; //�̰� �߿��� 2.
};

class bitrate_version_set {
public: //�׳� ���� public ����
	int index; // 0�� �츮�� ���� zencoder ����.

	int version_num;
	int version_set_num;

	int* resolution; //1080p�� ���� 1080��
	int* bitrate; // ���������� r�� ���� ��. �� ���⼱ kbps��, r�� Mbps(GHz ���� �� ��ȯ��). set_GHz �ֻ���� ������ ���� ����.
	double* data_size; 
	// �������� ������ �ܺ� ���ۿ� ���� ���� �ޱ� ������...  bitrate(kbps) -> MB/s�� ��ȯ��. 
	// GHz�� �ʴ� ��뷮�̴� ���ʵ� �ʴ� ������ ���۷����� �ϸ� �Ǵϱ�. �׷��Ƿ� �� ���� ������ ���۷��� �Ǵ� ��.

	int number_for_bit_opration;
	int set_versions_number_for_bit_opration;

	bitrate_version_set(int _index); //initiation.cpp�� ���� ����
};

/* channel.cpp */
void channel_initialization(channel* _channel_list, bitrate_version_set* _version_set, int _version_pop_type);
void set_VMAF(channel* _channel, bitrate_version_set* _version_set);
void set_GHz(channel* _channel, bitrate_version_set* _version_set);
void set_PWQ(channel* _channel, bitrate_version_set* _version_set);
double* set_gamma_pop(int length, double k, double theta);
double* set_version_pop(bitrate_version_set* _bitrate_version_set, int _version_pop_type);

/* server.cpp */
void server_initalization(server* _server_list);
double calculate_ES_cost(server* _server, double _total_transfer_data_size);
void set_coverage_infomation(channel* _channel_list, server* _server_list);
double calculate_distance(channel* _channel, server* _server);
double deg2rad(double deg);
double rad2deg(double rad);

/* algorithm.cpp */
void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);

/* comparison_schemes*/
void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set);
void method_RR_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void method_RR_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void method_RD_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void method_RD_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void method_CA_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);
void method_CA_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit);