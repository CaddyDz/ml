#ifndef CLASSIFIER_HPP
#define CLASSIFIER_HPP

#include <iostream>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

#include "data.hpp"
#include "treegenerator.hpp"

/**
 * Symbolic regression by genetic programming: evolves a population of trees that
 * fit reference samples.
 *
 * Each tree is scored with its root mean squared error (RMSE) over the training
 * samples given to s_data() (lower is better); a tree that gives NaN or an infinity
 * for a sample, or whose score is infinite, is invalid and discarded.
 *
 * Test samples given to s_test_data() are held back: they never take part in the
 * scoring nor the selection, and get_test_score() reports a tree's RMSE on them, to
 * check that a formula generalizes beyond the points it was evolved on. The first generation is drawn
 * at random. Each following one is bred from the previous generation:
 *   - the s_nb_elite() best trees are copied unchanged (elitism),
 *   - every other tree comes from parents chosen by tournament (the best of
 *     s_tournament_size() random trees): a crossover of two parents with probability
 *     s_crossover_rate(), a mutation of one parent with probability
 *     s_mutation_rate(), otherwise a plain copy of the parent.
 * Both operators come from the CTreeGenerator, so the children follow its Rules. A
 * generation holds s_nb_elm_per_gen() valid trees, sorted best first (equal scores:
 * smaller tree first); after 1000 x s_nb_elm_per_gen() draws it gives up with fewer
 * (e.g. none when a sample's result is NaN), and a new random generation is drawn if
 * the previous one is empty.
 *
 * The classifier owns its trees; the samples stay owned by the caller.
 */
class CClassifier
{
private:
	typedef std::vector<std::pair<Tree*,double> > Generation;

	int m_nb_elmt_per_gen=100;
	int m_nb_gen=1;
	int m_nb_elite=2;
	int m_tournament_size=4;
	double m_crossover_rate=0.7;
	double m_mutation_rate=0.25;
	bool m_verbose=false;
	std::mt19937 m_engine;
	std::vector<CData*> m_ref_data;
	std::vector<CData*> m_test_data;
	std::vector<Generation> m_generations;

	double rmse(Tree*, const std::vector<CData*>&) const;
	double score(Tree* tree) const{return this->rmse(tree, m_ref_data);}
	std::size_t tournament(std::size_t);
	Tree* breed(const Generation&, CTreeGenerator*);

public:
	CClassifier();
	~CClassifier();
	CClassifier(const CClassifier&) = delete;
	CClassifier& operator=(const CClassifier&) = delete;

	void s_data(CData* data){m_ref_data.push_back(data);}            ///< adds a training sample (not owned)
	void s_test_data(CData* data){m_test_data.push_back(data);}      ///< adds a held-back test sample (not owned)
	void s_nb_elm_per_gen(int nb_elm){m_nb_elmt_per_gen = nb_elm;}  ///< trees per generation, 100 by default
	void s_nb_gen(int nb_gen){m_nb_gen = nb_gen;}                   ///< generations computed by evolve(), 1 by default

	// Evolution settings
	void s_nb_elite(int nb){m_nb_elite = std::max(nb, 0);}                         ///< 2 by default
	void s_tournament_size(int size){m_tournament_size = std::max(size, 1);}        ///< 4 by default
	void s_crossover_rate(double rate){m_crossover_rate = std::min(std::max(rate, 0.0), 1.0);}  ///< 0.7 by default
	void s_mutation_rate(double rate){m_mutation_rate = std::min(std::max(rate, 0.0), 1.0);}    ///< 0.25 by default
	void s_seed(unsigned int seed){m_engine.seed(seed);}   ///< restarts the selection's random sequence (seed the generator too)
	void s_verbose(bool verbose){m_verbose = verbose;}      ///< logs every tree drawn ("NB TREES: n ERROR: score"), off by default

	void compute_new_generation(CTreeGenerator*);   ///< appends a generation: random for the first one, bred afterwards
	void evolve(CTreeGenerator*);                   ///< appends s_nb_gen() generations
	int nb_generations() const{return static_cast<int>(m_generations.size());}

	void print_ref_data() const;
	void print_generation(int i);                   ///< address and score of each tree of generation i, best first

	/// Tree of rank tr_id (0 = best) in generation gen_id, nullptr if out of range;
	/// still owned by the classifier.
	Tree* get_tree_for_specific_generation(int tr_id, int gen_id);

	/// Score (training RMSE) of the tree of rank tr_id in generation gen_id, NaN if out of range.
	double get_score(int tr_id, int gen_id) const;

	/// RMSE of the tree of rank tr_id in generation gen_id on the test samples; NaN if
	/// out of range, without test samples, or if the tree gives no valid value for one.
	/// Report it, but do not pick trees by it: the test samples would then take part
	/// in the selection.
	double get_test_score(int tr_id, int gen_id) const;
};

#endif // CLASSIFIER_HPP
