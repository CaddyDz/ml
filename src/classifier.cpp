#include "classifier.hpp"

CClassifier::~CClassifier()
{
	for (auto generation: m_generations)
	{
		for(auto tree_with_error: generation)
		{
			if (tree_with_error.first != nullptr) delete tree_with_error.first;
		}
	}
}

void CClassifier::print_ref_data() const
{
	for(auto data: m_ref_data)
	{
		const std::map<string,double>& variables = data->get_data();

		for(auto var:variables)
			std::cout << "("<<var.first << " = " << var.second << "),";

		std::cout << data->get_result() << std::endl;
	}

	return;
}

void CClassifier::compute_new_generation(CTreeGenerator* tgen)
{
	std::map<Tree*,double> trees_with_error;

	unsigned int nb_trees = 1;

	while (nb_trees <=m_nb_elmt_per_gen)
	{
		Tree* tree = tgen->generate_tree();

	   double error = 0.0;

		for (int j=0; j<m_ref_data.size(); ++j)
		{
			CData* data = m_ref_data[j];
			double result = tree->compute_value(data->get_data());

			if (!std::isnan(result) && !std::isinf(result))
			{
				//std::cout << "result: "<<result<<std::endl;
				error += (data->get_result() - result)*(data->get_result() - result);
			}
			else
			{
				// we ignore the tree
				error = -1.0;
				break;
			}
		}

		std::cout << "NB TREES: "<< nb_trees << " ERROR: "<<error << std::endl;

		if (error >= 0)
		{
			// no need of sqrt - the later is injective
			trees_with_error.emplace(std::pair<Tree*,double>(tree,sqrt(error)));
			++nb_trees;
		}
	}

	m_generations.push_back(trees_with_error);

	return;
}

void CClassifier::print_generation(int i)
{
	if (i<0 || i > m_generations.size()) {
		return;
	}
	else
	{
		for(auto generation:m_generations[i]) {
			std::cout << "Tree address: " << generation.first << " error value: " << generation.second << std::endl;
		}
	}

	return;
}

Tree* CClassifier::get_tree_for_specific_generation(int tr_id, int gen_id)
{
	if (gen_id < 0 || gen_id > m_generations.size())
		return nullptr;

	std::map<Tree*,double> gen = m_generations[gen_id];

	if (tr_id <0 || tr_id > gen.size())
		return nullptr;

	auto it = gen.begin();

	std::advance(it,gen_id);

	return (*it).first;
}
