#include "head.h"
int main() {
	srand(SEED);

	//��� ������ Ʈ�����ڵ��ϰ� ������ ����� ������ ��� �Ƿ��� ^_^;;;
	//�켱 ���� ������ ���� ���� ���� �ؼ� ��������.
	int cost_limit = 2000000; // 
	int pop_type = MVP;
	//�� ���� ���ڵ��� ���� ȯ�濡 ���� ����

	printf("===== START =====\n");
	printf("���� ���� - k �� : %lf, ��Ÿ �� : %lf\n\n", K_gamma, THETA_gamma);
	printf("���� �� : %d, ä�� �� : %d\n\n", ES_NUM, CHANNEL_NUM);

	server server_list[ES_NUM + 1];
	channel channel_list[CHANNEL_NUM + 1];
	bitrate_version_set version_set(0);
	channel_initialization(channel_list, &version_set, pop_type);
	server_initalization(server_list);
	set_coverage_infomation(channel_list, server_list);

	clock_t start, end, spent_time;
	start = clock();
	algorithm_run(server_list, channel_list, &version_set, cost_limit);
	end = clock();
	spent_time = end - start;
	printf("%lf second\n\n", (double)spent_time / CLOCKS_PER_SEC);

	//�񱳽�Ŵ ���� �ڵ� �� ��

	printf("===== �� ��Ŵ =====\n\n");
	comparison_schemes(GHz_WF_AP, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(GHz_WF_HPF, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(GHz_WF_VSD, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(cost_WF_AP, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(cost_WF_HPF, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(cost_WF_VSD, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(CA_AP, server_list, channel_list, &version_set, cost_limit);
	comparison_schemes(CA_HPF, server_list, channel_list, &version_set, cost_limit);

	printf("===== FINISH =====\n");
}