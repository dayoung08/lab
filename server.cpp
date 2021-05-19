#include "head.h"

//https://www.techrepublic.com/article/aws-snowball-edge-vs-azure-stack-what-on-premises-public-cloud-means-for-your-data-center/
//���� ������ ��ǥ���� 2�� (��� �ٸ� ������ ����� ������ ���Ѵ� ����)

//1. Snowball Edge
//_total_transfer_data_size ���� https://aws.amazon.com/ko/snowball/pricing/ 
//GHz ���� // instance
//���� https://docs.aws.amazon.com/ko_kr/snowball/latest/developer-guide/device-differences.html -> �ھ�� �� ����
//�� �뿩��.
//1) ���ϸ� ��� 2) ������ �۽� (������ �� �ȵ��)	//0.05 USD/GB * 25TB * 1,024 = 1,280 USD
// 0.0005 USD/MB 

//2. MS Azure Stack Edge 
// https://azure.microsoft.com/ko-kr/pricing/details/azure-stack/edge/
// ���⼭ ���� -> ��� ���� -> ����̽� ���, CPU �����θ� ������ 3���� ����̽��� ����.
// _total_transfer_data_size ���� : ���� �� �̰� �������� -_-;;;
// GHz ���� : �� ����̽������� ������ ����̹Ƿ� 0���� ���� �� ��.
// �� �뿩��

void server_initalization(server* _server_list) {
	//0���� ingression server
	//Hewlett Packard Enterprise Synergy 660 Gen10 Compute Module http://www.spec.org/power_ssj2008/results/res2019q2/power_ssj2008-20190311-00885.html
	//���� ���� 10�� �ִٰ� ��������.
	double edge_server_max_GHz[ES_TYPE_NUM + 1] = { 18144, 28.8, 108.8,  52.8, 44, 20.8}; // �Ҽ��� ���� �� �� 3517
	//1. Snowball Edge Storage Optimized(EC2 ��ǻ�� ��� ����) - Intel Xeon D ���μ���, 16�ھ�, 1.8Ghz
	//2. Snowball Edge Compute Optimized - AMD Naples, 32�ھ�, 3.4Ghz
	//3. Azure Stack Edge Pro - 2 X Intel Xeon �ǹ� 4214 CPU, 2.20 GHz, 24 �� ������ �ھ�(CPU �� 12 ��)
	//4. Azure Stack Edge Pro R - 2 X Intel Xeon �ǹ� 4114 CPU, 2.20GHz, 20���� ������ �ھ�(CPU �� 10 ��)
	//5. Azure Stack Edge Mini R - Intel Xeon-D 1577, 1.3GHz, 16�ھ�

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

//��� ����
double calculate_ES_cost(server* _server, double _total_transfer_data_size) {
	int bn_type = (_server->index - 1) % ES_TYPE_NUM + 1;
	//https://support.google.com/youtube/answer/2853702?hl=ko �̰� ���� DASH������ ���� ���׸�Ʈ�� 2���ε�. ����ϰ� 1�ʷ� ����. �ֳ��ϸ� 1�ʿ��� ����� ���ϴ�(������ 1���� ������ �� ���̴� ^^....)

	// �ϴ� ��� �뿩���� ���ִ� ���񽺸� �����....................
	// 3, 4, 5�� ���� ���� �ݾ��̹Ƿ� ������ 30��. 717, 2358, 1368
	double basic_charge[ES_TYPE_NUM + 1] = { 0, 80, 150, 23.9, 78.6, 45.6 }; // �Ϸ� �������� �� �޷��ΰ�
	double transmission_charge[ES_TYPE_NUM + 1] = { 0, 0.04, 0.04, 0, 0 }; // GB�� �� �޷��ΰ�

	double cost = PERIOD * (basic_charge[bn_type] + _total_transfer_data_size * transmission_charge[bn_type] * 3600 * 24);
	return cost;
}

//�Ʒ��� ���� Ŀ������ ����
void set_coverage_infomation(channel* _channel_list, server* _server_list) { // �������� �ֽ� �ڵ���.
	FILE* fp;
	   
	// ���� �� �ڵ忡�� ������ �Ҵ� ������ ������ ���ڸ� capacity�� ��Ÿ���� �̿� ���� �ڵ尡 ������, �츮�� GHz�� Cost�� �� ������ �ϹǷ� ������� ����.
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
		_channel_list[ch].available_server_list[0] = true; // ingestion server�� (index 0) ������ coverage�� ��.
		for (int ES = 1; ES <= ES_NUM; ES++) { // ���� �ڵ忡�� ���� ���ڿ���
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