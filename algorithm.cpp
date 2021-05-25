#include "head.h"

//�˰��� ���⼭���� ���� ¥����
short selected_set[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
short selected_ES[CHANNEL_NUM + 1];// �� ä���� � es���� �Ҵ�Ǿ��°�.
//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.

double remained_GHz[ES_NUM + 1];// processing capacity[es] - remained_GHz[es] �ϸ� used_GHz[es] ����. ��� ����� ���� GHz ����� ����.
double total_transfer_data_size[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����

short ES_count[ES_NUM + 1];

void algorithm_run(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	memset(ES_count, 0, (sizeof(short) * (ES_NUM + 1)));
	double first_GHz_temp = 0; //lowest version�� Ʈ�����ڵ��Ҷ�
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		first_GHz_temp += _channel_list[ch].sum_of_version_set_GHz[1];
	}

	double GHz_limit = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		remained_GHz[ES] = 0;
		GHz_limit += _server_list[ES].processing_capacity;
		total_transfer_data_size[ES] = 0;
	}

	if (GHz_limit < first_GHz_temp) {
		printf("GHz�� ���ڶ� ��Ȳ/Channel ���� ���̰ų�, ���� ���� �ø� ��\n");
		printf("lowest version�� Ʈ�����ڵ� ���� �� %lf GHz / %lf GHz\n\n", first_GHz_temp, GHz_limit);
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

	double total_GHz_temp = 0;
	double total_pwq_temp = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz_temp += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		total_pwq_temp += _channel_list[ch].sum_of_pwq[selected_set[ch]];
	}
	std::printf("=�߰�����= total_GHz_temp : %lf GHz, total_pwq_temp : %lf\n", total_GHz_temp, total_pwq_temp);
	std::printf("=��� ���� ������ �� �� GHz : %lf GHz\n\n", GHz_limit);

	// 2-1. CA-initialization phase
	set<pair<double, int>, greater<pair<double, int>> > list_CA_initialization;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		list_CA_initialization.insert(make_pair(slope, ch));
	}

	set<pair<double, int>, greater<pair<double, int>>> highest_GHz_first; // set�� ���� �ڵ� ������ �Ǿ� ���� ���� GHz�� ���� ���� ������ �� ���� ��.
	double highest_GHz_first_array[ES_NUM + 1]; // set���δ� GHz ���濡 ���� update�� �� �����ؼ� ���� array�� �����ؼ� update�� ����.

	//�� ������� �Լ� ������ ��. �׷��� ���� �� ���ϴ�.
	for (int ES = 1; ES <= ES_NUM; ES++) {
		highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity, ES)); //set
		highest_GHz_first_array[ES] = _server_list[ES].processing_capacity; //array
	}

	while (list_CA_initialization.size()) {
		int ch = (*list_CA_initialization.begin()).second; // slope�� ���� ū ���� � ä���ΰ�?
		list_CA_initialization.erase(list_CA_initialization.begin());//�� �� ������

		int queue_cnt = 1;
		int confirm_cnt = 1;
		short unavailable_ES_queue[ES_NUM+1];
		memset(unavailable_ES_queue, 0, (sizeof(short) * (ES_NUM + 1)));

		while (!highest_GHz_first.empty()) {
			int es = (*highest_GHz_first.begin()).second;
			if (!_channel_list[ch].available_server_list[es]) {
				unavailable_ES_queue[queue_cnt] = es;
				queue_cnt++;
				highest_GHz_first.erase(*highest_GHz_first.begin());
			}
			confirm_cnt++;
			if (confirm_cnt > ES_NUM){
				break;
			}
		}

		//Ŀ������ ������ �����ϴ� ES�� �����ϱ� ����.
		//���� highest_remained_utility_first�� ����ٸ� �̰� �׳� ingestion server�� ������.
		if (!highest_GHz_first.empty()) { // ���õ� ��尡 ���� �� ���� �ʾҴٸ�
			int ES = (*highest_GHz_first.begin()).second;
			if (remained_GHz[ES] + _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]] <= _server_list[ES].processing_capacity) {// ingestion server
				double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
				selected_ES[ch] = ES;
				remained_GHz[ES] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
				
				highest_GHz_first.erase(*highest_GHz_first.begin());
				highest_GHz_first.insert(make_pair(_server_list[ES].processing_capacity - remained_GHz[ES], ES)); //���õ� ������ �Ҵ�
				highest_GHz_first_array[ES] = _server_list[ES].processing_capacity - remained_GHz[ES]; //array ����

				total_transfer_data_size[ES] += _channel_list[ch].sum_of_transfer_data_size[selected_set[ch]];

				ES_count[ES]++;
			}
			else { // ingestion server
				selected_ES[ch] = 0;
				ES_count[0]++;
			}
		}
		else { // ingestion server
			selected_ES[ch] = 0;
			ES_count[0]++;
		}

		//highest_remained_utility_first�� �ٽ� Ŀ������ �� �¾Ҵ� ES�� ����.
		for (int cnt = 1; cnt < queue_cnt; cnt++) {
			highest_GHz_first.insert(make_pair(highest_GHz_first_array[unavailable_ES_queue[cnt]], unavailable_ES_queue[cnt]));
		}
	}


	// 2-2. CA-migration phase
	double total_cost = 0;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		if (ES_count[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
		}
	}
	//remained_GHz Ȯ���غ���

	set<pair<double, int>, greater<pair<double, int>> > list_CA_migration;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		if (selected_ES[ch] != 0) {
			double slope = _channel_list[ch].sum_of_pwq[selected_set[ch]] / calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
			list_CA_migration.insert(make_pair(slope, ch));
		}
	}

	while (total_cost > cost_limit) {
		if (!list_CA_migration.size()) {
			break;
		}

		int ch = (*list_CA_migration.begin()).second; // slope�� ���� ū ���� � ä���ΰ�?
		list_CA_migration.erase(list_CA_migration.begin());//�� �� ������


		double prev_cost =  calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
		total_transfer_data_size[selected_ES[ch]] -= _channel_list[ch].sum_of_transfer_data_size[selected_set[ch]];
		double curr_cost = calculate_ES_cost(&(_server_list[selected_ES[ch]]), total_transfer_data_size[selected_ES[ch]] / 1024);
		

		ES_count[selected_ES[ch]]--;
		ES_count[0]++;
		selected_ES[ch] = 0;

		remained_GHz[selected_ES[ch]] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		remained_GHz[0] -= _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
		
		total_cost -= (prev_cost - curr_cost);
	}

	//finalization�� �˰��� �󿡼� temp�� -> ��¥ �� Ȯ���ϴ� �����̶� ���⼱ �ʿ� x
	//but �Ʒ��� �� ��� ���� �Ҵ�� ä�ε��� �����Ϸ��� �ϴ� ����.
	double total_pwq = 0;
	double total_video_quality = 0;
	double used_GHz[ES_NUM + 1];

	/*int* number_of_transcoding = (int*)malloc(sizeof(int) * (_version_set->version_num));
	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		number_of_transcoding[ver] = 0;
	}*/
	// �� ��Ʈ����Ʈ ���� �� ���� transcoding�� ���� ��� ��. �� ������ range�� [0, CHANNEL_NUM]

	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz[ES] = 0;
	}
	std::printf("<<VDA-Greedy>>\n");
	//printf("[ä��-��Ʈ]\n");

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set[ch]];
		total_video_quality += _channel_list[ch].sum_of_video_quality[selected_set[ch]];
		used_GHz[selected_ES[ch]] += _channel_list[ch].sum_of_version_set_GHz[selected_set[ch]];
	}

	std::printf("\n��ü pwq�� �� : %lf", total_pwq);
	// �̰� ���̰� �߰��������� ���� pwq�� ���̰� ���� ��µ� �� �� ������, ���� �Ҵ��� ���� ������ �ſ� ���� pwq�� ������ �����̴�. �� ���̴� �ִµ� �Ҽ��� ©�ȴ�. 
	std::printf("\n��ü video_quality�� ��� : %lf\n\n", total_video_quality / CHANNEL_NUM / _version_set->version_num);
	/*for (int per_index = 0; per_index < 10; per_index++) {
		printf("%d%~%d%% pwq�� ��/video_quality�� ��� : %lf, %lf\n", (per_index * 10), (per_index + 1) * 10, pwq_sum_range[per_index], video_quality_sum_range[per_index] / (CHANNEL_NUM / 10) / info->version_num);
	}*/

	double sum_cost_final = 0;
	double sum_GHz_final = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if(ES > 0 && ES_count[ES] > 0)
			sum_cost_final += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size[ES] / 1024);
		sum_GHz_final += used_GHz[ES];
	}

	std::printf("\n��� ��� : %lf $\n", sum_cost_final);
	std::printf("��� GHz : %lf GHz\n\n", sum_GHz_final);

	/*for (int version = _version_set->version_num - 1; version >= 2; version--) {
		printf("���� %d : %d\n", version, number_of_transcoding[version]);
	}
	printf("\n");*/
}