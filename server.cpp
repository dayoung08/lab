#include "head.h"

//https://www.techrepublic.com/article/aws-snowball-edge-vs-azure-stack-what-on-premises-public-cloud-means-for-your-data-center/
//엣지 서비스의 대표적인 2곳 (사실 다른 곳들은 제대로 지원을 안한다 아직)

//1. Snowball Edge
//_total_transfer_data_size 가격 https://aws.amazon.com/ko/snowball/pricing/ 
//GHz 가격 // instance
//스펙 https://docs.aws.amazon.com/ko_kr/snowball/latest/developer-guide/device-differences.html -> 코어수 다 있음
//일 대여료.
//1) 데일리 요금 2) 데이터 송신 (수신은 돈 안든다)	//0.05 USD/GB * 25TB * 1,024 = 1,280 USD
// 0.0005 USD/MB 

//2. MS Azure Stack Edge 
// https://azure.microsoft.com/ko-kr/pricing/details/azure-stack/edge/
// 여기서 개념 -> 사양 검토 -> 디바이스 사양, CPU 종류로만 따지면 3가지 디바이스가 있음.
// _total_transfer_data_size 가격 : 없음 아 이게 정상이지 -_-;;;
// GHz 가격 : 월 디바이스에서의 무제한 계산이므로 0으로 봐도 될 듯.
// 월 대여료

void server_initalization(server* _server_list) {
	//0번은 ingression server
	//Hewlett Packard Enterprise Synergy 660 Gen10 Compute Module http://www.spec.org/power_ssj2008/results/res2019q2/power_ssj2008-20190311-00885.html
	//위의 것이 10대 있다고 가정하자.
	double edge_server_max_GHz[ES_TYPE_NUM + 1] = { 18144, 28.8, 108.8,  52.8, 44, 20.8}; // 소숫점 내림 총 합 3517
	//1. Snowball Edge Storage Optimized(EC2 컴퓨팅 기능 포함) - Intel Xeon D 프로세서, 16코어, 1.8Ghz
	//2. Snowball Edge Compute Optimized - AMD Naples, 32코어, 3.4Ghz
	//3. Azure Stack Edge Pro - 2 X Intel Xeon 실버 4214 CPU, 2.20 GHz, 24 개 물리적 코어(CPU 당 12 개)
	//4. Azure Stack Edge Pro R - 2 X Intel Xeon 실버 4114 CPU, 2.20GHz, 20개의 물리적 코어(CPU 당 10 개)
	//5. Azure Stack Edge Mini R - Intel Xeon-D 1577, 1.3GHz, 16코어

	for (int ES = 0; ES <= ES_NUM; ES++) {
		_server_list[ES].index = ES;
		_server_list[ES].total_cost = 0;
		_server_list[ES].total_GHz = 0;
		_server_list[ES].total_transfer_data_size = 0;

		int type = 0;
		if (ES > 0) {
			type = (ES - 1) % ES_TYPE_NUM + 1;
		}
		_server_list[ES].processing_capacity = edge_server_max_GHz[type];
	}
}

//비용 관련
double calculate_ES_cost(server* _server, double _total_transfer_data_size) {
	int bn_type = (_server->index - 1) % ES_TYPE_NUM + 1;
	//https://support.google.com/youtube/answer/2853702?hl=ko 이거 보면 DASH에서는 보통 세그먼트가 2초인듯. 깔끔하게 1초로 하자. 왜냐하면 1초여야 계산이 편하다(언젠가 1초인 세상이 올 것이다 ^^....)

	// 일단 기기 대여까지 해주는 서비스만 고려함....................
	// 3, 4, 5는 각각 월간 금액이므로 나누기 30함. 717, 2358, 1368
	double basic_charge[ES_TYPE_NUM + 1] = { 0, 80, 150, 23.9, 78.6, 45.6 }; // 하루 기준으로 몇 달러인가
	double transmission_charge[ES_TYPE_NUM + 1] = { 0, 0.04, 0.04, 0, 0 }; // GB당 몇 달러인가

	double cost = PERIOD * (basic_charge[bn_type] + _total_transfer_data_size * transmission_charge[bn_type] * 3600 * 24);
	return cost;
}

//아래는 전부 커버리지 관련
void set_coverage_infomation(channel* _channel_list, server* _server_list) { // 교수님이 주신 코드임.
	FILE* fp;
	   
	// 본래 이 코드에는 서버에 할당 가능한 유저의 숫자를 capacity로 나타내고 이에 대한 코드가 있으나, 우리는 GHz와 Cost가 이 역할을 하므로 사용하지 않음.
	// server read
	fopen_s(&fp, "servercoord.txt", "r");

	for (int ES = 1; ES <= ES_NUM; ES++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		_server_list[ES].server_location.latitude = latitude;
		_server_list[ES].server_location.longitude = longitude;
	}
	fclose(fp);

	// user read
	fopen_s(&fp, "usercoord.txt", "r");

	for (int ch = 0; ch < CHANNEL_NUM; ch++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		_channel_list[ch].broadcaster_location.latitude = latitude;
		_channel_list[ch].broadcaster_location.longitude = longitude;
	}
	fclose(fp);

	// generate random coverage range
	// set coverage for each edge server
	// range: 450m~750m
	srand((unsigned)time(NULL));
	_server_list[0].coverage = INF; // ingestion server.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		_server_list[ES].coverage = rand() % 450 + (750 - 450);
	}

	// assign
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel* ch_ptr = &(_channel_list[ch]);
		_channel_list[ch].available_server_list[0] = true; // ingestion server은 (index 0) 무조건 coverage에 들어감.
		for (int ES = 1; ES <= ES_NUM; ES++) { // 기존 코드에도 서버 숫자였다
			server* ES_ptr = &(_server_list[ES]);
			double dist = calculate_distance(ch_ptr, ES_ptr);
			// the user is at outside of coverage
			if (dist > _server_list[ES].coverage)
				_channel_list[ch].available_server_list[ES] = false;
			else
				_channel_list[ch].available_server_list[ES] = true;
		}
	}
}

// calculate the distance between an user and a server, and then find the user is in the server's coverage
// refer to http://www.movable-type.co.uk/scripts/latlong.html
// refer to https://www.geodatasource.com/developers/c
double calculate_distance(channel* _channel, server* _server) {
	double dist = 0.0; // distance between an user and a server
	double theta = 0.0;

	// if the same points
	if ((_channel->broadcaster_location.latitude == _server->server_location.latitude) && (_channel->broadcaster_location.longitude == _server->server_location.longitude))
		return 0;

	// if not
	else {
		theta = _channel->broadcaster_location.longitude - _server->server_location.longitude;
		dist = sin(deg2rad(_channel->broadcaster_location.latitude)) * sin(deg2rad(_server->server_location.latitude)) + cos(deg2rad(_channel->broadcaster_location.latitude)) * cos(deg2rad(_server->server_location.latitude)) * cos(deg2rad(theta));
		dist = acos(dist);
		dist = rad2deg(dist);
		dist = dist * 60 * 1.1515; // the unit is mile, so
		dist = dist * 1.609344; // convert to kilometers
		dist = dist * 1000.0; // convert to meters

		return dist;
	}
}
// degree to radian
double deg2rad(double deg) {
	return (deg * PI / 180.0);
}

// radian to degree
double rad2deg(double rad) {
	return (rad * 180.0 / (double)PI);
}