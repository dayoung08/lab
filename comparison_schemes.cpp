#include "head.h"

//전반적으로 확인 할 필요가 있음 20210515
//비교 스킴들이 구현되었다.
short selected_set_in_comparison_schemes[CHANNEL_NUM + 1]; // 각 채널에서 사용하는 비트레이트 set
short** selected_ES_in_comparison_schemes;//[CHANNEL_NUM + 1][VERSION_NUM]; // 각 채널에서 사용하는 비트레이트 set에 속하는 각 버전이 어떤 es에서 선택되었는가.
//오리지널 버전은 트랜스코딩 안해서 배열 크기가 저렇다.
double used_GHz_in_comparison_schemes[ES_NUM + 1];
double total_transfer_data_size_in_comparison_schemes[ES_NUM + 1];//실시간으로 전송하는 데이터 사이즈의 합 계산을 위해
short ES_count_in_comparison_schemes[ES_NUM + 1];

void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES_in_comparison_schemes = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz_in_comparison_schemes[ES] = 0;
		total_transfer_data_size_in_comparison_schemes[ES] = 0;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		selected_ES_in_comparison_schemes[ch] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // 오리지널 버전은 트랜스코딩 안하니까
		for (int ver = 1; ver < _version_set->version_num; ver++) {  // 오리지널 버전은 트랜스코딩 안하니까
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
	//처음에 version 1번 먼저 다 할당하고,
	//GHz, cost 제한에 맞을 때까지, 인기 높은 채널은 모든 set을 트랜스코딩.

	int ES = 1;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		//먼저 모든 채널을 set 1로 설정한다.
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
					//모든 엣지에 할당이 불가능한 상태임
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
		//먼저 모든 채널을 set 1로 설정한다.
		int ch = (*channal_popularity.begin()).second;
		channal_popularity.erase(channal_popularity.begin());
		//int set = _version_set->version_set_num;

		for (int ver = 2; ver <= _version_set->version_num-1; ver++) {
			bool is_allocated = false;
			int cnt = 1;

			//여기서부터 set에 속하는 것들을 1번 버전 빼고 2~N번버전까지 할당하자
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
						//모든 엣지에 할당이 불가능한 상태임
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
						//이미 할당된, 해당 채널의 다른 버전들의 할당을 전부 해제함.
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
//여기까지 수정

void method_RR_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0에서 시작하는거지 0이 백엔드 노드 인덱스가 되는게 아님

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		bool is_full[ES_NUM + 1];
		for (int bk = 1; bk <= ES_NUM; bk++) {
			is_full[bk] = false;
		}
		do {
			int confirm_cnt = 0;
			do {
				if (confirm_cnt == ES_NUM) { // 모든 엣지가 커버리지 내에 없다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
				ES++; //RR로 backend node 할당하기 위해 ++
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
				if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
			}//es에 ch의 1번 버전 트랜스코딩 태스트를 할당할 때, total_cost가 넘었는지 안 넘었는지 확인

			if (total_cost > cost_limit) {
				flag = false; //이제 할당 자체를 중지한다.2번~version_num-1번 버전은 할당하지 않음.
				//printf("2. RR_HPF() : 2번 다 못했는데 꽉 참\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps니까 GB 단위 변환이 필요하네...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1번 버전을 라운드 로빈 형태로 할당함.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //가장 평균 pop이 높은 버전을 찾기 위함
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // 버전 1은 이미 트랜스코딩했고 마지막버전은 원본이라 트랜스코딩 안함
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
	//여기까지 수정

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // 가장 높은 pop인 버전
		high_pop_version_first.erase(*high_pop_version_first.begin()); // 맨 위의 값 삭제
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			bool is_full[ES_NUM + 1];
			for (int bk = 1; bk <= ES_NUM; bk++) {
				is_full[bk] = false;
			}
			do {
				int confirm_cnt = 0;
				do {
					if (confirm_cnt == ES_NUM) { // 모든 엣지가 커버리지 내에 없다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						break;
					}
					ES++; //RR로 backend node 할당하기 위해 ++
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
					if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver]) < 0);// 해당 노드가 꽉 찼는지 아닌지 확인

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
				}//전력이 꽉 찼는지 아닌지 확인
				if (total_cost > cost_limit) {
					flag = false; //이제 할당 중지한다.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps니까 GB 단위 변환이 필요하네...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // 나머지 버전을 라운드 로빈 형태로 할당함.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}


void method_RD_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0에서 시작하는거지 0이 백엔드 노드 인덱스가 되는게 아님
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int bk = 1; bk <= ES_NUM; bk++) {
		is_full[bk] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			ES = rand() % ES_NUM + 1; //랜덤으로 백엔드 노드 할당

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
			}//전력이 꽉 찼는지 아닌지 확인
			if (total_cost > cost_limit) {
				flag = false; //이제 할당 중지한다.
				//printf("3. RD_AP() : 3번 다 못했는데 꽉 참\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps니까 GB 단위 변환이 필요하네...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1번 버전을 라운드 로빈 형태로 할당함.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //그 다음 2번 버전~ 6번 버전은 pop 순으로 라운드 로빈으로 할당함.
		int ch = (*high_pop_channel_first.begin()).second; //가장 높은 pop인 채널
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // 맨 위의 값 삭제

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				ES = rand() % ES_NUM + 1; //랜덤으로 백엔드 노드 할당

				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						//현재 ver 이전거 ~ 2번까지 할당한 걸 삭제한다.
						for (int v = (ver - 1); v >= 2; v--) {
							used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
				}//전력이 꽉 찼는지 아닌지 확인
				if (total_cost > cost_limit) {
					//현재 ver 이전거 ~ 2번까지 할당한 걸 삭제한다.
					for (int v = (ver - 1); v >= 2; v--) {
						used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //이제 할당 중지한다.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps니까 GB 단위 변환이 필요하네...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // 나머지 버전을 라운드 로빈 형태로 할당함.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}

void method_RD_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0에서 시작하는거지 0이 백엔드 노드 인덱스가 되는게 아님

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int full_node_cnt = 0;
		do {
			ES = rand() % ES_NUM + 1; //랜덤으로 백엔드 노드 할당

			if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					//현재 ver 이전거 ~ 2번까지 할당한 걸 삭제한다.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
			}//전력이 꽉 찼는지 아닌지 확인
			if (total_cost > cost_limit) {
				flag = false; //이제 할당 중지한다.
				//printf("4. RD_HPF() : 4번 다 못했는데 꽉 참\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps니까 GB 단위 변환이 필요하네...
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1번 버전을 라운드 로빈 형태로 할당함.
			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //가장 평균 pop이 높은 버전을 찾기 위함
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // 버전 1은 이미 트랜스코딩했고 마지막버전은 원본이라 트랜스코딩 안함
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
	//여기까지 세팅

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // 가장 높은 pop인 버전
		high_pop_version_first.erase(*high_pop_version_first.begin()); // 맨 위의 값 삭제
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				ES = rand() % ES_NUM + 1; //랜덤으로 백엔드 노드 할당

				if (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int es = 1; es <= ES_NUM; es++) {
						if (is_full[es])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
				}//전력이 꽉 찼는지 아닌지 확인
				if (total_cost > cost_limit) {
					flag = false; //이제 할당 중지한다.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps니까 GB 단위 변환이 필요하네...
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // 나머지 버전을 라운드 로빈 형태로 할당함.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}


void method_CA_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // 가장 사용 비용이 낮은 노드 선택

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0이 백엔드 노드 인덱스가 되는게 아님
	server* ES_ptr = &(_server_list[ES]);
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			if (!low_cost_first.size()) {
				flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 채널은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
				break;
			}
			ES = (*low_cost_first.begin()).second; // 가장 파워가 낮은 노드

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
				if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
			}//전력이 꽉 찼는지 아닌지 확인
			if (total_cost > cost_limit) {
				flag = false; //이제 할당 중지한다.
				//printf("1. RR_AP() : 1번 다 못했는데 꽉 참\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU 사용율에 따른 update를 하기 위해 삭제
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1번 버전을 가장 전력이 낮은 노드 먼저 할당함.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //그 다음 2번 버전~ 6번 버전은 pop 순으로 라운드 로빈으로 할당함.
		int ch = (*high_pop_channel_first.begin()).second; //가장 높은 pop인 채널
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // 맨 위의 값 삭제

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 채널은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
				ES = (*low_cost_first.begin()).second; // 가장 파워가 낮은 노드
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
					if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						//현재 ver 이전거 ~ 2번까지 할당한 걸 삭제한다.
						for (int v = (ver - 1); v >= 2; v--) {
							used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
				}//전력이 꽉 찼는지 아닌지 확인
				if (total_cost > cost_limit) {
					//현재 ver 이전거 ~ 2번까지 할당한 걸 삭제한다.
					for (int v = (ver - 1); v >= 2; v--) {
						used_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //이제 할당 중지한다.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU 사용율에 따른 update를 하기 위해 삭제

				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // 해당 버전을 가장 전력이 낮은 노드 먼저 할당함.
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES]) / 1024, ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}

void method_CA_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // 가장 사용 전력이 낮은 노드 선택

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0이 백엔드 노드 인덱스가 되는게 아님
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
				flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
				break;
			}
			ES = (*low_cost_first.begin()).second; // 가장 파워가 낮은 노드

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
				if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
			}
		} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
			}//전력이 꽉 찼는지 아닌지 확인
			if (total_cost > cost_limit) {
				flag = false; //이제 할당 중지한다.
				//printf("1. RR_AP() : 1번 다 못했는데 꽉 참\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU 사용율에 따른 update를 하기 위해 삭제
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1번 버전을 가장 전력이 낮은 노드 먼저 할당함.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
			ES_count_in_comparison_schemes[ES]++;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //가장 평균 pop이 높은 버전을 찾기 위함
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // 버전 1은 이미 트랜스코딩했고 마지막버전은 원본이라 트랜스코딩 안함
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
	//여기까지 세팅

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // 가장 높은 pop인 버전
		high_pop_version_first.erase(*high_pop_version_first.begin()); // 맨 위의 값 삭제
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
					break;
				}
				ES = (*low_cost_first.begin()).second; // 가장 파워가 낮은 노드
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
					if (full_node_cnt >= ES_NUM) { // 모든 백엔드가 꽉 차있다면
						flag = false; //이제 할당 중지한다. 이것보다 더 낮은 인기도의 버전은 만약 꽉 안찬다 쳐도, 그들을 고려할 필요는 없다.
						break;
					}
				}
			} while (is_full[ES] || (used_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// 해당 노드가 꽉 찼는지 아닌지 확인

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
				}//전력이 꽉 찼는지 아닌지 확인
				if (total_cost > cost_limit) {
					flag = false; //이제 할당 중지한다.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU 사용율에 따른 update를 하기 위해 삭제
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				used_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // 해당 버전을 가장 전력이 낮은 노드 먼저 할당함.
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_count_in_comparison_schemes[ES]++;
			}
		}
	}
}