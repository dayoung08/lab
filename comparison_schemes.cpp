#include "head.h"

//���������� Ȯ�� �� �ʿ䰡 ���� 20210515
//�� ��Ŵ���� �����Ǿ���.
short selected_set_in_comparison_schemes[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
short** selected_ES_in_comparison_schemes;//[CHANNEL_NUM + 1][VERSION_NUM]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set�� ���ϴ� �� ������ � es���� ���õǾ��°�.
//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.
double used_GHz_in_comparison_schemes[ES_NUM + 1];
double total_transfer_data_size_in_comparison_schemes[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����
short ES_count_in_comparison_schemes[ES_NUM + 1];

void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES_in_comparison_schemes = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz_in_comparison_schemes[ES] = 0;
		total_transfer_data_size_in_comparison_schemes[ES] = 0;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		selected_ES_in_comparison_schemes[ch] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int ver = 1; ver < _version_set->version_num; ver++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES_in_comparison_schemes[ch][ver] = 0;
		}
	}

	if (method_index == RR_AP) {
		method_RR_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RR_HPF) {
		method_RR_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RA_AP) {
		method_RD_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RA_HPF) {
		method_RD_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == CA_AP) {
		method_CA_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == CA_HPF) {
		method_CA_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}

	for (int channel = 1; channel <= CHANNEL_NUM; channel++) {
		int set = 1;
		for (int version = 2; version <= _version_set->version_num - 1; version++) {
			if (selected_ES_in_comparison_schemes[channel][version] != 0)
				set += (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (version - 1)));
		}
		selected_set_in_comparison_schemes[channel] = set;
	}
	print_method(method_index, _server_list, _channel_list, _version_set);
}

void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set) {
	if (method_index == RR_AP) {
		printf("<<RR_AP>>");
	}
	else if (method_index == RR_HPF) {
		printf("<<RR_HPF>>");
	}
	else if (method_index == RA_AP) {
		printf("<<RA_AP>>");
	}
	else if (method_index == RA_HPF) {
		printf("<<RA_HPF>>");
	}
	else if (method_index == CA_AP) {
		printf("<<PA_AP>>");
	}
	else if (method_index == CA_HPF) {
		printf("<<PA_HPF>>");
	}


	double total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set_in_comparison_schemes[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set_in_comparison_schemes[ch]];
	}
	double total_cost = 0;
	for (int ES = 0; ES <= ES_NUM; ES++) {
		if (ES_count_in_comparison_schemes[ES] > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size_in_comparison_schemes[ES] / 1024);
		}
	}
	std::printf(" total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);
}

void method_RR_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	//ó���� version 1�� ���� �� �Ҵ��ϰ�,
	//GHz, cost ���ѿ� ���� ������, �α� ���� ä���� ��� set�� Ʈ�����ڵ�.

	int ES = 1;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//���� ��� ä���� set 1�� �����Ѵ�.
		bool is_allocated = false;
		int cnt = 1;
		while (!is_allocated) {
			double GHz = _server_list[ES].processing_capacity - used_GHz_in_comparison_schemes[ES];
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				if (ES_count_in_comparison_schemes[es]) {
					if(es == ES)
						total_cost += calculate_ES_cost(&(_server_list[es]), (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					else
						total_cost += calculate_ES_cost(&(_server_list[es]), total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}
			}

			if ((!_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[1] >= 0) && total_cost <= cost_limit) {
				used_GHz_in_comparison_schemes[ES] += _channel_list[ch].video_GHz[1];
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
				
				selected_ES_in_comparison_schemes[ch][1] = ES;
				ES_count_in_comparison_schemes[ES]++;
				
				is_allocated = true;
			}
			else {
				if (cnt == ES_NUM) {
					//��� ������ �Ҵ��� �Ұ����� ������
					used_GHz_in_comparison_schemes[0] += _channel_list[ch].video_GHz[1];
					total_transfer_data_size_in_comparison_schemes[0] += _version_set->data_size[1];

					selected_ES_in_comparison_schemes[ch][1] = 0;
					ES_count_in_comparison_schemes[0]++;

					break;
				}
				else {
					if (ES == ES_NUM) {
						ES = 1;
					}
					else {
						ES++;
					}
					cnt++;
				}
			}
			selected_set_in_comparison_schemes[ES] = 1;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > channal_popularity;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		channal_popularity.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	while (!channal_popularity.empty()) {
		//���� ��� ä���� set 1�� �����Ѵ�.
		int ch = (*channal_popularity.begin()).second;
		channal_popularity.erase(channal_popularity.begin());
		//int set = _version_set->version_set_num;

		for (int ver = 2; ver <= _version_set->version_num-1; ver++) {
			bool is_allocated = false;
			int cnt = 1;

			//���⼭���� set�� ���ϴ� �͵��� 1�� ���� ���� 2~N���������� �Ҵ�����
			while (!is_allocated) {
				double GHz = _server_list[ES].processing_capacity - used_GHz_in_comparison_schemes[ES];
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					if (ES_count_in_comparison_schemes[es]) {
						if (es == ES)
							total_cost += calculate_ES_cost(&(_server_list[es]), (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						else
							total_cost += calculate_ES_cost(&(_server_list[es]), total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}

				if ((!_channel_list[ch].available_server_list[ES]) && (GHz - _channel_list[ch].video_GHz[ver] >= 0) && total_cost <= cost_limit) {
					used_GHz_in_comparison_schemes[ES] += _channel_list[ch].video_GHz[ver];
					total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
					
					selected_ES_in_comparison_schemes[ch][ver] = ES;
					ES_count_in_comparison_schemes[ES]++;
					is_allocated = true;
				}
				else {
					if (cnt == ES_NUM) {
						//��� ������ �Ҵ��� �Ұ����� ������
						used_GHz_in_comparison_schemes[0] += _channel_list[ch].video_GHz[ver];
						total_transfer_data_size_in_comparison_schemes[0] += _version_set->data_size[ver];

						selected_ES_in_comparison_schemes[ch][ver] = 0;
						ES_count_in_comparison_schemes[0]++;

						for (int v = (ver - 1); v >= 2; v--) {
							used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							
							selected_ES_in_comparison_schemes[ch][v] = 0;

							used_GHz_in_comparison_schemes[0] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[0] += _version_set->data_size[v];
							ES_count_in_comparison_schemes[0]++;
						}
						//�̹� �Ҵ��, �ش� ä���� �ٸ� �������� �Ҵ��� ���� ������.
						break;
					}
					else {
						if (ES == ES_NUM) {
							ES = 1;
						}
						else {
							ES++;
						}
						cnt++;
					}
				}
			}
		}
		selected_set_in_comparison_schemes[ES] = _version_set->version_set_num;
	}
}
//������� ����

void method_RR_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		bool is_full[ES_NUM + 1];
		for (int bk = 1; bk <= ES_NUM; bk++) {
			is_full[bk] = false;
		}
		do {
			int confirm_cnt = 0;
			do {
				if (confirm_cnt == ES_NUM) { // ��� ������ Ŀ������ ���� ���ٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
				ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
				if (ES > ES_NUM) {
					ES = 1;
				}
				confirm_cnt++;
			} while (!_channel_list[ch].available_server_list[ES]);

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES_count_in_comparison_schemes[es] > 0) {
					if (ES == es) {
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					}
					else {
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}
			}//es�� ch�� 1�� ���� Ʈ�����ڵ� �½�Ʈ�� �Ҵ��� ��, total_cost�� �Ѿ����� �� �Ѿ����� Ȯ��

			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� ��ü�� �����Ѵ�.2��~version_num-1�� ������ �Ҵ����� ����.
				//printf("2. RR_HPF() : 2�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		high_pop_version_first.insert(make_pair(version_pop_average[ver], ver));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			bool is_full[ES_NUM + 1];
			for (int bk = 1; bk <= ES_NUM; bk++) {
				is_full[bk] = false;
			}
			do {
				int confirm_cnt = 0;
				do {
					if (confirm_cnt == ES_NUM) { // ��� ������ Ŀ������ ���� ���ٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
					ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
					if (ES > ES_NUM) {
						ES = 1;
					}
					confirm_cnt++;
				} while (!_channel_list[ch].available_server_list[ES]);

				if ((used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver]) < 0) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver]) < 0);// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES_count_in_comparison_schemes[es] > 0) {
						if (ES == es) {
							total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						}
						else {
							total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
						}
					}
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}


void method_RD_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int bk = 1; bk <= ES_NUM; bk++) {
		is_full[bk] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES_count_in_comparison_schemes[es] > 0) {
					if (ES == es) {
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					}
					else {
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("3. RD_AP() : 3�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //�� ���� 2�� ����~ 6�� ������ pop ������ ���� �κ����� �Ҵ���.
		int ch = (*high_pop_channel_first.begin()).second; //���� ���� pop�� ä��
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // �� ���� �� ����

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
						for (int v = (ver - 1); v >= 2; v--) {
							used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES_count_in_comparison_schemes[es] > 0) {
						if (ES == es) {
							total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						}
						else {
							total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
						}
					}
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					for (int v = (ver - 1); v >= 2; v--) {
						used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}

void method_RD_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int full_node_cnt = 0;
		do {
			ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]); 
				if (ES_count_in_comparison_schemes[es] > 0) {
					if (ES == es) {
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					}
					else {
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("4. RD_HPF() : 4�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int version = 2; version <= _version_set->version_num - 1; version++) {
		high_pop_version_first.insert(make_pair(version_pop_average[version], version));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int es = 1; es <= ES_NUM; es++) {
						if (is_full[es])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES_count_in_comparison_schemes[es] > 0) {
						if (ES == es) {
							total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						}
						else {
							total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
						}
					}
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}


void method_CA_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // ���� ��� ����� ���� ��� ����

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	server* ES_ptr = &(_server_list[ES]);
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			if (!low_cost_first.size()) {
				flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ä���� ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
				break;
			}
			ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				low_cost_first.erase(*low_cost_first.begin());
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES_count_in_comparison_schemes[es] > 0) {
					if (ES == es) {
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					}
					else {
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("1. RR_AP() : 1�� �� ���ߴµ� �� ��\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU ������� ���� update�� �ϱ� ���� ����
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� ������ ���� ��� ���� �Ҵ���.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //�� ���� 2�� ����~ 6�� ������ pop ������ ���� �κ����� �Ҵ���.
		int ch = (*high_pop_channel_first.begin()).second; //���� ���� pop�� ä��
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // �� ���� �� ����

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ä���� ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
				ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���
				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					low_cost_first.erase(*low_cost_first.begin());
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
						for (int v = (ver - 1); v >= 2; v--) {
							used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES_count_in_comparison_schemes[es] > 0) {
						if (ES == es) {
							total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						}
						else {
							total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
						}
					}
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					for (int v = (ver - 1); v >= 2; v--) {
						used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU ������� ���� update�� �ϱ� ���� ����

				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // �ش� ������ ���� ������ ���� ��� ���� �Ҵ���.
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES]) / 1024, ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}

void method_CA_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // ���� ��� ������ ���� ��� ����

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	server* ES_ptr = &(_server_list[ES]);
	int full_node_cnt = 0;

	bool is_full[ES_NUM + 1];
	for (int bk = 1; bk <= ES_NUM; bk++) {
		is_full[bk] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int full_node_cnt = 0;
		do {
			if (!low_cost_first.size()) {
				flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
				break;
			}
			ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				low_cost_first.erase(*low_cost_first.begin());
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					if (is_full[es])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES_count_in_comparison_schemes[es] > 0) {
					if (ES == es) {
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
					}
					else {
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
					}
				}
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("1. RR_AP() : 1�� �� ���ߴµ� �� ��\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU ������� ���� update�� �ϱ� ���� ����
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� ������ ���� ��� ���� �Ҵ���.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int version = 2; version <= _version_set->version_num - 1; version++) {
		high_pop_version_first.insert(make_pair(version_pop_average[version], version));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
				ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���
				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					low_cost_first.erase(*low_cost_first.begin());
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES_count_in_comparison_schemes[es] > 0) {
						if (ES == es) {
							total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
						}
						else {
							total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
						}
					}
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU ������� ���� update�� �ϱ� ���� ����
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // �ش� ������ ���� ������ ���� ��� ���� �Ҵ���.
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}