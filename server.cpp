#include "head.h"

//https://www.techrepublic.com/article/aws-snowball-edge-vs-azure-stack-what-on-premises-public-cloud-means-for-your-data-center/
//엣지 서비스의 대표적인 2곳 (사실 다른 곳들은 제대로 지원을 안한다 아직)

// 아래는 Snowball Edge(1, 2), MS Azure Stack Edge(3, 4, 5) 의 금액을 초당으로 변경한 것.
// 1, 2는 일간 금액으므로 나누기 24 * 3600함. 80, 150
// 3, 4, 5는 각각 월간 금액이므로 나누기 30 * 24 * 3600함. 717, 2358, 1368
double full_charge[ES_TYPE_NUM + 1] = { 0, 0.000926, 0.00174, 0.000277, 0.00091, 0.000528 };

//0번은 CTS
//Hewlett Packard Enterprise Synergy 660 Gen10 Compute Module http://www.spec.org/power_ssj2008/results/res2019q2/power_ssj2008-20190311-00885.html
//위의 것이 10대 있다고 가정하자.
double edge_server_max_GHz[ES_TYPE_NUM + 1] = { 1814.4, 28.8, 108.8, 52.8, 44, 20.8 }; // 소숫점 내림 총 합 3517
//1. Snowball Edge Storage Optimized(EC2 컴퓨팅 기능 포함) - Intel Xeon D 프로세서, 16코어, 1.8Ghz
//2. Snowball Edge Compute Optimized - AMD Naples, 32코어, 3.4Ghz
//3. Azure Stack Edge Pro - 2 X Intel Xeon 실버 4214 CPU, 2.20 GHz, 24 개 물리적 코어(CPU 당 12 개)
//4. Azure Stack Edge Pro R - 2 X Intel Xeon 실버 4114 CPU, 2.20GHz, 20개의 물리적 코어(CPU 당 10 개)
//5. Azure Stack Edge Mini R - Intel Xeon-D 1577, 1.3GHz, 16코어


void server_initalization(server* _server_list) {
	for (int ES = 0; ES <= ES_NUM; ES++) {
		_server_list[ES].index = ES;
		/*_server_list[ES].total_cost = 0;
		_server_list[ES].total_GHz = 0;
		_server_list[ES].total_transfer_data_size = 0;*/

		int type = 0;
		if (ES > 0) {
			type = (ES - 1) % ES_TYPE_NUM + 1;
		}
		_server_list[ES].processing_capacity = edge_server_max_GHz[type];
	}
}

//비용 관련
double calculate_ES_cost(server* _server, double _used_GHz, int _model) { //초당 cost
	int bn_type = (_server->index - 1) % ES_TYPE_NUM + 1;
	//https://support.google.com/youtube/answer/2853702?hl=ko 이거 보면 DASH에서는 보통 세그먼트가 2초인듯. 깔끔하게 1초로 하자. 왜냐하면 1초여야 계산이 편하다(언젠가 1초인 세상이 올 것이다 ^^....)

	double cost = 0;
	if (_model == CPU_USAGE_MODEL) {
		// 일단 기기 대여까지 해주는 서비스만 고려함....................
		// 3, 4, 5는 각각 월간 금액이므로 나누기 30함. 717, 2358, 1368
		double percent = _used_GHz / _server->processing_capacity;
		//double cost = (full_charge[bn_type] * percent) * (PERIOD * 24 * 3600);
		cost = full_charge[bn_type] * percent;
	}
	else if (_model == LEASING_MODEL) {
		if (_used_GHz)
			cost = full_charge[bn_type];
		else
			cost = 0;
	}
	else if (_model == STEP_MODEL) {
		double percent = _used_GHz / _server->processing_capacity;
		double step = 0;
		
		if (percent) {
			if (percent == 100) {
				step = 1;
			}
			else
				step = ceil(percent * 10) / 10;
		}
		else
			step = 0.1;

		cost = full_charge[bn_type] * step;
	}
	return cost;
}



double get_full_charge() {
	double full_total_charge = 0;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		int bn_type = (ES - 1) % ES_TYPE_NUM + 1;
		full_total_charge += full_charge[bn_type];
	}
	return full_total_charge;
}

//아래는 전부 커버리지 관련
void set_coverage_infomation(channel* _channel_list, server* _server_list) { // 교수님이 주신 코드임.
	FILE* fp;
	   
	// 본래 이 코드에는 서버에 할당 가능한 유저의 숫자를 capacity로 나타내고 이에 대한 코드가 있으나, 우리는 GHz와 Cost가 이 역할을 하므로 사용하지 않음.
	// server read
	fopen_s(&fp, "servercoord.txt", "r");

	for (int ES = 1; ES <= 125; ES++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		
		for (int cnt = 0; cnt < (ES_NUM / 125); cnt++) {
			_server_list[ES + 125 * cnt].server_location.latitude = latitude;
			_server_list[ES + 125 * cnt].server_location.longitude = longitude;
			//cout << ES + 125 * cnt << endl;
		}
	}
	fclose(fp);

	// user read
	fopen_s(&fp, "usercoord.txt", "r");

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		_channel_list[ch].broadcaster_location.latitude = latitude;
		_channel_list[ch].broadcaster_location.longitude = longitude;
	}
	fclose(fp);

	// generate random coverage range
	// set coverage for each edge server
	// range: 450m~750m
	srand(SEED);
	_server_list[0].coverage = INF; // CTS.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		_server_list[ES].coverage = rand() % 301 + 450;
	}

	// assign
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel* ch_ptr = &(_channel_list[ch]);
		_channel_list[ch].available_server_list[0] = true; // CTS은 (index 0) 무조건 coverage에 들어감.
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
double deg2rad(double _deg) {
	return (_deg * PI / 180.0);
}

// radian to degree
double rad2deg(double _rad) {
	return (_rad * 180.0 / (double)PI);
}