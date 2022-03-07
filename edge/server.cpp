#include "head.h"

//시뮬레이션 환경 설정
//0번은 CTS
//Hewlett Packard Enterprise Synergy 660 Gen10 Compute Module http://www.spec.org/power_ssj2008/results/res2019q2/power_ssj2008-20190311-00885.html

double ES_GHz[NUM_OF_MACHINE + 1] = { 0, 864, 1254.4, 256, 324.8, 313.6 };
double ES_one_core_GHz[NUM_OF_MACHINE + 1] = { 0, 2.25, 2.45, 2.00, 2.90, 2.45 };
//1. ASUSTeK Computer Inc. RS620SA-E10-RS12 https://www.spec.org/power_ssj2008/results/res2020q4/power_ssj2008-20200918-01046.html
//2. Hewlett Packard Enterprise Apollo XL225n Gen10 Plus https://www.spec.org/power_ssj2008/results/res2021q1/power_ssj2008-20210223-01073.html
//3. Dell Inc. PowerEdge R7525 http://www.spec.org/power_ssj2008/results/res2020q2/power_ssj2008-20200324-01021.html
//4. Fujitsu PRIMERGY RX4770 M6 https://www.spec.org/power_ssj2008/results/res2020q4/power_ssj2008-20201006-01049.html
//5. Lenovo Global Technology ThinkSystem SR665 https://www.spec.org/power_ssj2008/results/res2021q2/power_ssj2008-20210408-01094.html

void server_initalization(server* _server_list, int _model, bool _bandwidth_model_flag) {
	_server_list[0].index = 0;
	//_server_list[0].machine_type = 0;
	_server_list[0].cost_model_type = 0;

	_server_list[0].processing_capacity = 1;
	_server_list[0].maximum_bandwidth = 1;
	_server_list[0].cpu_usage_cost_alpha = 0;
	_server_list[0].bandwidth_cost_alpha = 0;


	mt19937 random_generation(SEED);
	//normal_distribution<double> normal_distribution_for_cpu_usage_cost_alpha(5.209248167, 0.727618028);
	//normal_distribution<double> normal_distribution_for_bandwidth_cost_alpha(0.352295981, 0.308087043);

	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		_server_list[ES].index = ES;
		//_server_list[ES].machine_type = rand() % NUM_OF_MACHINE + 1;
		int pos = rand() % NUM_OF_MACHINE + 1;
		_server_list[ES].processing_capacity = ES_GHz[pos];
		_server_list[ES].processing_capacity /= CTS_GHz;
		_server_list[ES].cpu_usage_cost_alpha = ((double)(rand() % 38 + 63)) / 100;
	}	
}

void server_initalization_for_bandwidth(server* _server_list, int _model, bool _bandwidth_model_flag) {
	//220225
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		if (_bandwidth_model_flag) {
			_server_list[ES].maximum_bandwidth = rand() % 71 + 30; //https://docs.vmware.com/en/VMware-SD-WAN/services/sd-wan-aws-virtual-edge-deployment-guide/GUID-6D5BAC8C-5CFA-4564-A9A8-A92267779A96.html
			//_server_list[ES].maximum_bandwidth = rand() % 999001 + 1000; // 아마존 기준 
			_server_list[ES].maximum_bandwidth /= CTS_Mbps;
			_server_list[ES].bandwidth_cost_alpha = ((double)(rand() % 993 + 8)) / 1000;
			//_server_list[ES].cpu_usage_cost_alpha = _server_list[ES].bandwidth_cost_alpha = max(_server_list[ES].bandwidth_cost_alpha, _server_list[ES].cpu_usage_cost_alpha);
		}
		else {
			_server_list[ES].maximum_bandwidth = INFINITY;
			_server_list[ES].bandwidth_cost_alpha = 0;
		}
	}
}
//비용 관련
double calculate_ES_cost(server* _server, double _used_GHz, double _used_Mbps, int _model) {
	double GHz_cost = calculate_ES_cpu_usage_cost(_server, _used_GHz, _model);
	double Mbps_cost = calculate_ES_bandwidth_cost(_server, _used_Mbps, _model);
	return max(GHz_cost, Mbps_cost);
}

double calculate_ES_cpu_usage_cost(server* _server, double _used_GHz, int _model) { //초당 cost
	double cost = 0;
	if (_model == LINEAR_MODEL) {
		cost = _server->cpu_usage_cost_alpha * (_used_GHz / _server->processing_capacity);
	}
	else if (_model == ONOFF_MODEL) {
		if (_used_GHz > 0)
			cost = _server->cpu_usage_cost_alpha;
		else
			cost = 0;
	}
	return cost;
}


double calculate_ES_bandwidth_cost(server* _server, double _used_Mbps, int _model) { //초당 cost
	double cost = 0;
	if (_model == LINEAR_MODEL) {
		cost = _server->bandwidth_cost_alpha * (_used_Mbps / _server->maximum_bandwidth);
	}
	else if (_model == ONOFF_MODEL) {
		if (_used_Mbps > 0)
			cost = _server->bandwidth_cost_alpha;
		else
			cost = 0;
	}
	return cost;
}

double get_total_charge(server* _server_list, int _cost_model) {
	double full_total_charge = 0;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		full_total_charge += max(_server_list[ES].cpu_usage_cost_alpha, _server_list[ES].bandwidth_cost_alpha);
	}
	return full_total_charge;
}

//아래는 전부 커버리지 관련
void set_coverage_infomation(channel* _channel_list, server* _server_list) { // 교수님이 주신 코드임.
	FILE* fp;

	// 본래 이 코드에는 서버에 할당 가능한 유저의 숫자를 capacity로 나타내고 이에 대한 코드가 있으나, 우리는 GHz와 Cost가 이 역할을 하므로 사용하지 않음.
	// server read
	fopen_s(&fp, "servercoord.txt", "r");

	pair<double, double> coverage[126];
	for (int ES = 1; ES <= 125; ES++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);

		coverage[ES].first = latitude;
		coverage[ES].second = longitude;
	}
	fclose(fp);

	//210717 버그 수정
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		double latitude = coverage[(ES - 1) % 125 + 1].first;
		double longitude = coverage[(ES - 1) % 125 + 1].second;

		_server_list[ES].server_location.latitude = latitude;
		_server_list[ES].server_location.longitude = longitude;
	}

	// user read
	fopen_s(&fp, "usercoord.txt", "r");

	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		_channel_list[ch].broadcaster_location.latitude = latitude;
		_channel_list[ch].broadcaster_location.longitude = longitude;
	}
	fclose(fp);

	// generate random coverage range
	// set coverage for each edge server
	// range: 450m~750m
	_server_list[0].coverage = INF; // CTS.
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		_server_list[ES].coverage = rand() % 301 + 450;
	}

	// assign
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		channel* ch_ptr = &(_channel_list[ch]);
		_channel_list[ch].available_server_list[0] = true; // CTS은 (index 0) 무조건 coverage에 들어감.
		for (int ES = 1; ES <= NUM_OF_ES; ES++) { // 기존 코드에도 서버 숫자였다
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