#include "classifier.hpp"

#include <algorithm>
#include <ctime>
#include <limits>

CClassifier::CClassifier()
{
	// like the generator: random_device may be deterministic, so mix in the time
	// and a count of the classifiers
	static unsigned int created = 0;
	std::random_device rd;
	m_engine.seed(rd() ^ static_cast<unsigned int>(std::time(nullptr)) ^ (++created * 2246822519u));
}

CClassifier::~CClassifier()
{
	for (const auto& generation: m_generations)
	{
		for(const auto& tree_with_error: generation)
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

double CClassifier::rmse(Tree* tree, const std::vector<CData*>& samples) const
{
	// root mean squared error, -1 for an invalid prediction (0 without samples)
	double error = 0.0;

	for (std::size_t j=0; j<samples.size(); ++j)
	{
		CData* data = samples[j];
		double result = tree->compute_value(data->get_data());

		if (!std::isnan(result) && !std::isinf(result))
		{
			// hypot accumulates the root of the sum of squares without overflowing it
			error = std::hypot(error, data->get_result() - result);
		}
		else
			return -1.0;
	}

	return samples.empty() ? 0.0 : error / std::sqrt(static_cast<double>(samples.size()));
}

std::size_t CClassifier::tournament(std::size_t population)
{
	// a generation is sorted best first: the smallest of the ranks drawn wins
	std::uniform_int_distribution<std::size_t> rank(0, population-1);
	std::size_t best = rank(m_engine);

	for (int i = 1; i < m_tournament_size; ++i)
		best = std::min(best, rank(m_engine));

	return best;
}

Tree* CClassifier::breed(const Generation& parents, CTreeGenerator* tgen)
{
	std::uniform_real_distribution<double> uniform(0.0, 1.0);
	double draw = uniform(m_engine);
	const Tree& mother = *parents[this->tournament(parents.size())].first;

	if (draw < m_crossover_rate)
		return tgen->crossover(mother, *parents[this->tournament(parents.size())].first);

	if (draw < m_crossover_rate + m_mutation_rate)
		return tgen->mutate(mother);

	return mother.clone();
}

void CClassifier::compute_new_generation(CTreeGenerator* tgen)
{
	Generation trees_with_error;

	// the first generation (or one after an empty generation) is drawn at random
	const bool breeding = !m_generations.empty() && !m_generations.back().empty();

	if (breeding)
	{
		// elitism: copies of the best trees (each generation owns its own trees),
		// scored again in case samples were added since
		const Generation& previous = m_generations.back();

		for (int i = 0; i < m_nb_elite && i < static_cast<int>(previous.size()) && i < m_nb_elmt_per_gen; ++i)
		{
			Tree* elite = previous[i].first->clone();
			double error = this->score(elite);

			if (error >= 0 && std::isfinite(error))
				trees_with_error.emplace_back(elite, error);
			else
				delete elite;
		}
	}

	// trees are drawn until enough of them are valid: give up if nearly none is
	// (e.g. when a reference result is NaN, every error is)
	long long attempts = 0;
	const long long max_attempts = 1000LL * std::max(m_nb_elmt_per_gen, 1);

	while (static_cast<int>(trees_with_error.size()) < m_nb_elmt_per_gen)
	{
		if (++attempts > max_attempts)
		{
			std::cerr << "Only " << trees_with_error.size() << " valid trees found in " << max_attempts << " attempts" << std::endl;
			break;
		}

		Tree* tree = breeding ? this->breed(m_generations.back(), tgen) : tgen->generate_tree();
		double error = this->score(tree);

		if (m_verbose)
			std::cout << "NB TREES: "<< trees_with_error.size()+1 << " ERROR: "<<error << std::endl;

		// a score too large for a double is as invalid as an infinite prediction
		if (error >= 0 && std::isfinite(error))
			trees_with_error.emplace_back(tree, error);
		else
			delete tree;
	}

	// best first; equal scores: the smaller tree first
	std::stable_sort(trees_with_error.begin(), trees_with_error.end(),
					 [](const std::pair<Tree*,double>& a, const std::pair<Tree*,double>& b)
					 {
						 if (a.second != b.second)
							 return a.second < b.second;
						 return a.first->nodes().size() < b.first->nodes().size();
					 });

	m_generations.push_back(trees_with_error);

	return;
}

void CClassifier::evolve(CTreeGenerator* tgen)
{
	for (int i = 0; i < m_nb_gen; ++i)
		this->compute_new_generation(tgen);
}

void CClassifier::print_generation(int i)
{
	if (i<0 || i >= static_cast<int>(m_generations.size())) {
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
	if (gen_id < 0 || gen_id >= static_cast<int>(m_generations.size()))
		return nullptr;

	const Generation& gen = m_generations[gen_id];

	if (tr_id <0 || tr_id >= static_cast<int>(gen.size()))
		return nullptr;

	return gen[tr_id].first;
}

double CClassifier::get_score(int tr_id, int gen_id) const
{
	if (gen_id < 0 || gen_id >= static_cast<int>(m_generations.size()) ||
		tr_id < 0 || tr_id >= static_cast<int>(m_generations[gen_id].size()))
		return std::numeric_limits<double>::quiet_NaN();

	return m_generations[gen_id][tr_id].second;
}

double CClassifier::get_test_score(int tr_id, int gen_id) const
{
	if (m_test_data.empty() || gen_id < 0 || gen_id >= static_cast<int>(m_generations.size()) ||
		tr_id < 0 || tr_id >= static_cast<int>(m_generations[gen_id].size()))
		return std::numeric_limits<double>::quiet_NaN();

	double error = this->rmse(m_generations[gen_id][tr_id].first, m_test_data);

	return (error >= 0) ? error : std::numeric_limits<double>::quiet_NaN();
}
