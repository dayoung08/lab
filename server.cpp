#include "head.h"

//�ùķ��̼� ȯ�� ����
//0���� CTS
//Hewlett Packard Enterprise Synergy 660 Gen10 Compute Module http://www.spec.org/power_ssj2008/results/res2019q2/power_ssj2008-20190311-00885.html
//���� ���� 10�� �ִٰ� ��������.


// [Linear model : CPU ��뷮�� ���� ������ �����Ǵ� ����]
// AWS�� Google�� ���� ���� �� ������ ���߱� ���� "�ð��� ��� (����� ��ŭ ����)"�� �����ؼ� ����� (�̰� ���� ���� �ð� �����⵵ ��)
double hourly_charge_for_linear_model[NUM_OF_LINEAR_COST_MODEL + 1] = { 0, 0.0208, 0.0416, 0.063, 0.01096, 0.03288 };
// 1���� vcpu�� double �޷��� �ҿ��. 
//1. AWS Wavelength (t3.medium) - 2 vCPU, 0.0416 USD
//2. AWS Wavelength (t3.xlarge) - 4 vCPU, 0.1664 USD
//3. AWS Wavelength (r5.2xlarge) - 8 vCPU, 0.504 USD
//4. Google Cloud Anthos, �ۺ� Ŭ���忡�� Anthos ��� - 1 vCPU, 0.01096 USD
//5. Google Cloud Anthos, �������̽�(On-premise) ȯ�濡�� Anthos ��� - 1 vCPU, 0.03288 USD

int ES_number_of_vCPUs_for_linear_model[NUM_OF_MACHINES_FOR_LINEAR_MODEL + 1] = { 0, 768, 1024, 256, 224, 256 };
double ES_GHz_for_linear_model[NUM_OF_MACHINES_FOR_LINEAR_MODEL + 1] = { 1814.4, 864, 1254.4, 256, 324.8, 313.6};
//1. ASUSTeK Computer Inc. RS620SA-E10-RS12 https://www.spec.org/power_ssj2008/results/res2020q4/power_ssj2008-20200918-01046.html -> vcpu : 768��
//2. Hewlett Packard Enterprise Apollo XL225n Gen10 Plus https://www.spec.org/power_ssj2008/results/res2021q1/power_ssj2008-20210223-01073.html -> vcpu: 1024��
//3. Dell Inc. PowerEdge R7525 http://www.spec.org/power_ssj2008/results/res2020q2/power_ssj2008-20200324-01021.html -> vcpu: 256��
//4. Fujitsu PRIMERGY RX4770 M6 https://www.spec.org/power_ssj2008/results/res2020q4/power_ssj2008-20201006-01049.html -> vcpu: 224��
//5. Lenovo Global Technology ThinkSystem SR665 https://www.spec.org/power_ssj2008/results/res2021q2/power_ssj2008-20210408-01094.html -> vcpu: 256��


// onoff model : onoff�ϴ� ����̽��� ���� ������ �����Ǵ� ����.
// Snowball Edge(1, 2), MS Azure Stack Edge(3, 4, 5) �� ���� �ݾ�, ����캼 ������ �ϰ� �ݾ��̹Ƿ� x 30�� ��. (���� �� ������ ���߱� ����)
double monthly_charge_for_onoff_model[NUM_OF_MACHINES_FOR_ONOFF_MODEL + 1] = { 0, 2400, 4500, 717, 2358, 1368 };
double ES_GHz_for_onoff_model[NUM_OF_MACHINES_FOR_ONOFF_MODEL + 1] = { 1814.4, 28.8, 108.8, 52.8, 44, 20.8 }; // �Ҽ��� ���� �� �� 3517

//1. Snowball Edge Storage Optimized(EC2 ��ǻ�� ��� ����) - Intel Xeon D ���μ���, 16�ھ�, 1.8Ghz
//2. Snowball Edge Compute Optimized - AMD Naples, 32�ھ�, 3.4Ghz
//3. Azure Stack Edge Pro - 2 X Intel Xeon �ǹ� 4214 CPU, 2.20 GHz, 24 �� ������ �ھ�(CPU �� 12 ��)
//4. Azure Stack Edge Pro R - 2 X Intel Xeon �ǹ� 4114 CPU, 2.20GHz, 20���� ������ �ھ�(CPU �� 10 ��)
//5. Azure Stack Edge Mini R - Intel Xeon-D 1577, 1.3GHz, 16�ھ�

void server_initalization(server* _server_list, int _model) {
	_server_list[0].index = 0;
	_server_list[0].machine_type = 0;
	_server_list[0].cost_model_type = 0;
	_server_list[0].processing_capacity = 1814.4;

	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		_server_list[ES].index = ES;

		if (_model == CPU_USAGE_MODEL) {
			_server_list[ES].machine_type = rand() % NUM_OF_LINEAR_COST_MODEL + 1;
			_server_list[ES].cost_model_type = rand() % NUM_OF_LINEAR_COST_MODEL + 1;
			_server_list[ES].processing_capacity = ES_GHz_for_linear_model[_server_list[ES].machine_type];
		}
		else if (_model == ONOFF_MODEL) {
			_server_list[ES].machine_type = rand() % NUM_OF_MACHINES_FOR_ONOFF_MODEL + 1;
			_server_list[ES].cost_model_type = rand() % NUM_OF_ONOFF_COST_MODEL + 1;
			_server_list[ES].processing_capacity = ES_GHz_for_onoff_model[_server_list[ES].machine_type];
		}
	}
}

//��� ����
double calculate_ES_cost(server* _server, double _used_GHz, int _model) { //�ʴ� cost
	double cost = 0;
	if (_model == CPU_USAGE_MODEL) {
		double full_charge = ES_number_of_vCPUs_for_linear_model[_server->machine_type] * hourly_charge_for_linear_model[_server->cost_model_type];
		double percent = _used_GHz / _server->processing_capacity;
		cost = full_charge * percent;
	}
	else if (_model == ONOFF_MODEL) {
		if (_used_GHz)
			cost = monthly_charge_for_onoff_model[_server->machine_type];
		else
			cost = 0;
	}

	return cost;

	//log ���̶� exponential �� �𵨵� ���� 
}

double get_full_charge(server* _server_list, int _cost_model) {
	double full_total_charge = 0;
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		if (_cost_model == CPU_USAGE_MODEL) {
			full_total_charge += ES_number_of_vCPUs_for_linear_model[_server_list[ES].machine_type] * hourly_charge_for_linear_model[_server_list[ES].cost_model_type];
		}
		else if (_cost_model == ONOFF_MODEL) {
			full_total_charge += monthly_charge_for_onoff_model[_server_list[ES].machine_type];
		}
	}
	return full_total_charge;
}

//�Ʒ��� ���� Ŀ������ ����
void set_coverage_infomation(channel* _channel_list, server* _server_list) { // �������� �ֽ� �ڵ���.
	FILE* fp;
	   
	// ���� �� �ڵ忡�� ������ �Ҵ� ������ ������ ���ڸ� capacity�� ��Ÿ���� �̿� ���� �ڵ尡 ������, �츮�� GHz�� Cost�� �� ������ �ϹǷ� ������� ����.
	// server read
	fopen_s(&fp, "servercoord.txt", "r");

	for (int ES = 1; ES <= 125; ES++) {
		double latitude, longitude;

		fscanf(fp, "%lf\t%lf\n", &latitude, &longitude);
		
		for (int cnt = 0; cnt < (NUM_OF_ES / 125); cnt++) {
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
	for (int ES = 1; ES <= NUM_OF_ES; ES++) {
		_server_list[ES].coverage = rand() % 301 + 450;
	}

	// assign
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channel* ch_ptr = &(_channel_list[ch]);
		_channel_list[ch].available_server_list[0] = true; // CTS�� (index 0) ������ coverage�� ��.
		for (int ES = 1; ES <= NUM_OF_ES; ES++) { // ���� �ڵ忡�� ���� ���ڿ���
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