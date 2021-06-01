#include "head.h"

//�˰��� ���⼭���� ���� ¥����
short selected_set[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
//short selected_ES[CHANNEL_NUM + 1];// �� ä���� � es���� �Ҵ�Ǿ��°�.
//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.

short** selected_ES;

double remained_GHz[ES_NUM + 1];// processing capacity[es] - remained_GHz[es] �ϸ� used_GHz[es] ����. ��� ����� ���� GHz ����� ����.
double used_GHz[ES_NUM + 1];
double total_transfer_data_size[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����

short ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int row = 1; row <= CHANNEL_NUM; row++) {
		selected_ES[row] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int col = 1; col <= _version_set->version_num-1; col++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES[row][col] = -1;
		}
	}

	memset(ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	double first_GHz = 0; //lowest version�� Ʈ�����ڵ��Ҷ�
	
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz += _channel_list[ch].sum_of_version_set_GHz[1];
	}
	for (int ES = 0; ES <= ES_NUM; ES++) {
		remained_GHz[ES] = 0;
		total_transfer_data_size[ES] = 0;
	}

	//�� ES�� Ŀ�������� Ȯ���ϰ� ��ü ä�� �� �� ��� ä�ο��� �Ҵ��� �������� �ۼ��������� ���ϰ�, �� �� processing capacity�� ���غ���.
	double GHz_limit = _server_list[0].processing_capacity;
	//double GHz_limit_2 = _server_list[0].processing_capacity;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		int alloc_ch_cnt = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			if (_channel_list[ch].available_server_list[ES]) {
				alloc_ch_cnt++;
			}
		}

		//GHz_limit += _server_list[ES].processing_capacity * (((double)alloc_ch_cnt) / CHANNEL_NUM);
		GHz_limit += _server_list[ES].processing_capacity;
	}

	//������� 210530 ����. coverage�� ������ processing capacity�� ��ֶ����� ����.

	printf("lowest version�� Ʈ�����ڵ� ���� �� %lf GHz / GHz �� �� %lf GHz\n\n", first_GHz, GHz_limit);
	if (GHz_limit < first_GHz) {
		printf("GHz�� ���ڶ� ��Ȳ/Channel ���� ���̰ų�, ���� ���� �ø� ��\n");
		exit(0);
	}


	//���߿� �� ������� �Լ� ������ ��. �׷��� ���� �� ���ϴ�.
	//1. VSD phase
	double total_GHz = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//selected_ES[ch] = 0;
		selected_set[ch] = _version_set->version_set_num;
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num];
	}
	set<pair<double, pair<int, int>>, less<pair<double, pair<int, int>>> > list_VSD;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int set = 1; set <= _version_set->version_set_num - 1; set++) { //�ҽ��� ���� 1080p
			double slope = (_channel_list[ch].sum_of_pwq[_version_set->version_set_num] - _channel_list[ch].sum_of_pwq[set]) / (_channel_list[ch].sum_of_version_set_GHz[_version_set->version_set_num] - _channel_list[ch].sum_of_version_set_GHz[set]);
			list_VSD.insert(make_pair(slope, make_pair(ch, set)));
		}
	}

	while (list_VSD.size()) {
		int ch = (*list_VSD.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int set = (*list_VSD.begin()).second.second; //slope�� ���� ū ���� � ��Ʈ�ΰ�?

		list_VSD.erase(list_VSD.begin());//�� �� ������
		//int prev_����_node = selected_BN[channel];
		int prev_set = selected_set[ch];
		if (_channel_list[ch].sum_of_version_set_GHz[set] < _channel_list[ch].sum_of_version_set_GHz[prev_set]) {
			double expected_total_GHz = total_GHz - _channel_list[ch].sum_of_version_set_GHz[prev_set] + _channel_list[ch].sum_of_version_set_GHz[set];
			total_GHz = expected_total_GHz;
			selected_set[ch] = set;
			if (expected_total_GHz < GHz_limit) {
				break;
			}
		}
	}

	total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}

	std::printf("=VSD= total_GHz : %lf GHz, total_pwq : %lf\n", total_GHz, total_pwq);

	// 2-1. CA-initialization phase
	set<pair<double, int>, greater<pair<double, int>>> highest_GHz_first; // set�� ���� �ڵ� ������ �Ǿ� ���� ���� GHz�� ���� ���� ������ �� ���� ��.
	double highest_GHz_first_array[ES_NUM + 1]; // set���δ� GHz ���濡 ���� update�� �� �����ؼ� ���� array�� �����ؼ� update�� ����.

	//�� ������� �Լ� ������ ��. �׷��� ���� �� ���ϴ�.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
		highest_GHz_first_array[ES] = _server_list[ES].processing_capacity; //array
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int ES = (*highest_GHz_first.begin()).second; // ���� ���� GHz�� ���� �鿣��� �����ΰ�?
		float GHz = (*highest_GHz_first.begin()).first; // �� �鿣���� GHz�� ���ΰ�?

		if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
			selected_ES[ch][1] = ES;

			highest_GHz_first.erase(highest_GHz_first.begin());
			highest_GHz_first.insert(make_pair((GHz - _channel_list[ch].video_GHz[1]), ES)); //set ����
			highest_GHz_first_array[ES] = (GHz - _channel_list[ch].video_GHz[1]); //map ����

			remained_GHz[ES] += _channel_list[ch].video_GHz[1];
		}
		else {
			selected_ES[ch][1] = 0;
			ES_count[0]++;
			remained_GHz[0] += _channel_list[ch].video_GHz[1];

			if (remained_GHz[0] >= _server_list[0].processing_capacity) {
				//printf("���� �߻���, �Ҵ��� ä�� ���� %d\n", alloc_num);
			}
		}
	}

	set<pair<long float, pair<int, int>>, greater<pair<long float, pair<int, int>>> > list_CA_initialization;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = selected_set[ch];
		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
			if ((set - 1) & (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1)))) { // ������ ������ set���� �Ҵ��ߴ� GHz�� ���� ������ �ش�. 
				float slope = (_channel_list[ch].pwq[ver] / (_channel_list[ch].video_GHz[ver]));
				list_CA_initialization.insert(make_pair(slope, make_pair(ch, ver)));
				//cnt++;
			}
		}
	}
	int last_version_num = 0;
	while (list_CA_initialization.size()) {
		int ch = (*list_CA_initialization.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver = (*list_CA_initialization.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
		list_CA_initialization.erase(list_CA_initialization.begin());//�� �� ������

		int queue_cnt = 1;
		int confirm_cnt = 1;
		short unavailable_ES_queue[ES_NUM + 1];
		memset(unavailable_ES_queue, 0, (sizeof(short) * (ES_NUM + 1)));

		//alloc_num++;
		while (!highest_GHz_first.empty()) {
			int es = (*highest_GHz_first.begin()).second;
			if (!_channel_list[ch].available_server_list[es]) {
				unavailable_ES_queue[queue_cnt] = es;
				queue_cnt++;
				highest_GHz_first.erase(*highest_GHz_first.begin());
			}
			confirm_cnt++;
			if (confirm_cnt > ES_NUM) {
				break;
			}
		}

		//Ŀ������ ������ �����ϴ� ES�� �����ϱ� ����.
		//���� highest_remained_utility_first�� ����ٸ� �̰� �׳� ingestion server�� ������.
		if (!highest_GHz_first.empty()) { // ���õ� ��尡 ���� �� ���� �ʾҴٸ�
			int ES = (*highest_GHz_first.begin()).second;
			if (remained_GHz[ES] + _channel_list[ch].video_GHz[ver] <= _server_list[ES].processing_capacity) {// ingestion server
				selected_ES[ch][ver] = ES;
				remained_GHz[ES] += _channel_list[ch].video_GHz[ver];

				highest_GHz_first.erase(*highest_GHz_first.begin());
				highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity - remained_GHz[ES], ES)); //���õ� ������ �Ҵ�
				highest_GHz_first_array[ES] = _server_list[ES].processing_capacity - remained_GHz[ES]; //array ����

				total_transfer_data_size[ES] += _version_set->data_size[ver];
				ES_count[ES]++;
			}
			else { // ingestion server
				if (remained_GHz[0] < _server_list[0].processing_capacity) {
					selected_ES[ch][ver] = 0;
					ES_count[0]++;
					remained_GHz[0] += _channel_list[ch].video_GHz[ver];
				}
			}
		}
		else { // ingestion server
			if (remained_GHz[0] < _server_list[0].processing_capacity) {
				selected_ES[ch][ver] = 0;
				ES_count[0]++;
				remained_GHz[0] += _channel_list[ch].video_GHz[ver];
			}
		}

		//highest_remained_utility_first�� �ٽ� Ŀ������ �� �¾Ҵ� ES�� ����.
		for (int cnt = 1; cnt < queue_cnt; cnt++) {
			highest_GHz_first.insert(make_pair(highest_GHz_first_array[unavailable_ES_queue[cnt]], unavailable_ES_queue[cnt]));
		}
	}

	//set ����ϱ�
	recalculate_set(_version_set);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	double total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			used_GHz[ES] = _server_list[ES].processing_capacity - remained_GHz[ES];
		}
	}
	std::printf("=CA-init= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);


	// 2-2. CA-migration phase
	// �ƴ�. ���� �������. 
	// migration�� �ƴϰ� �Ҵ�� version �߿��� ������.
	set<pair<long float, pair<int, int>>, less<pair<long float, pair<int, int>>> > list_CA_exception;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double cost = 0;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] > 0) { //-1�� �Ҵ� �� ��, 0�� ingestion server
				double slope = _channel_list[ch].pwq[ver] / calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
				list_CA_exception.insert(make_pair(slope, make_pair(ch, ver)));
			}
		}
	}

	while (list_CA_exception.size()) {
		if (total_cost < cost_limit) {
			break;
		}

		int ch = (*list_CA_exception.begin()).second.first; // slope�� ���� ū ���� � ä���ΰ�?
		int ver = (*list_CA_exception.begin()).second.second; // slope�� ���� ū ���� � �����ΰ�?
		list_CA_exception.erase(list_CA_exception.begin());//�� �� ������

		if (remained_GHz[selected_ES[ch][ver]] < 0) {
			cout << "error";
		}

		if (selected_ES[ch][ver] > 0) {
			double prev_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);
			total_transfer_data_size[selected_ES[ch][ver]] -= _version_set->data_size[ver];
			double curr_cost = calculate_ES_cost(&(_server_list[selected_ES[ch][ver]]), total_transfer_data_size[selected_ES[ch][ver]] / 1024);


			ES_count[selected_ES[ch][ver]]--;
			remained_GHz[0] += _channel_list[ch].video_GHz[ver];

			if (!ES_count[selected_ES[ch][ver]]) {
				remained_GHz[selected_ES[ch][ver]] = 0;
				total_cost -= prev_cost;
			}
			else {
				remained_GHz[selected_ES[ch][ver]] -= _channel_list[ch].video_GHz[ver];
				total_cost -= (prev_cost - curr_cost);
			}

			selected_ES[ch][ver] = -1;
		}
	}

	//set ����ϱ�
	recalculate_set(_version_set);

	total_GHz = 0;
	total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
			used_GHz[ES] = _server_list[ES].processing_capacity - remained_GHz[ES];
		}
	}
	std::printf("=����= total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);
}

void recalculate_set(bitrate_version_set* _version_set) {
	//set ����ϱ�
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int set = 0;
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			if (selected_ES[ch][ver] != -1)
				set += _version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (ver - 1));
			//�� ����ϰ� +1�Ұ�
		}
		set += 1;
		selected_set[ch] = set;
	}
}