/*

   Copyright (c) 2006-2010, The Scripps Research Institute

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Author: Dr. Oleg Trott <ot14@columbia.edu>, 
           The Olson Lab, 
           The Scripps Research Institute

*/

#include "quasi_newton.h"
#include "bfgs.h"
#include "conf_gpu.h"

struct quasi_newton_aux {
	model* m;
	const precalculate* p;
	const igrid* ig;
	const vec v;
	const grid* user_grid;
	quasi_newton_aux(model* m_, const precalculate* p_, const igrid* ig_, const vec& v_, const grid* user_grid_) : m(m_), p(p_), ig(ig_), v(v_), user_grid(user_grid_) {}
	fl operator()(const conf& c, change& g) {
		const fl tmp = m->eval_deriv(*p, *ig, v, c, g, *user_grid);
		return tmp;
	}
	
	fl operator()(const conf_gpu& c, change_gpu& g) {
		const fl tmp = m->eval_deriv(*p, *ig, v, c, g, *user_grid);
		return tmp;
	}
};

void quasi_newton::operator()(model& m, const precalculate& p, const igrid& ig, output_type& out, change& g, const vec& v, const grid& user_grid, bool gpu_on) const { // g must have correct size
	quasi_newton_aux aux(&m, &p, &ig, v, &user_grid);
	// check whether we're using the gpu for the minimization algorithm. if so, malloc and
	// copy change, out, and aux here. then call bfgs with the gpu out,aux, and change, which will
	// automatically use the gpu for the routine via templating of bfgs
	if (gpu_on) {
		/*change_gpu chgpu;
		cudaMalloc(&chgpu, sizeof(change_gpu));
		cudaMemcpy(chgpu, &g, sizeof(change), cudaMemcpyHostToDevice);

		output_type_gpu outgpu;
		cudaMalloc(&outgpu, sizeof(output_type_gpu));
		cudaMemcpy(outgpu, &out, sizeof(output_type), cudaMemcpyHostToDevice);

		quasi_newton_aux aux_gpu;
		cudaMalloc(&aux_gpu, sizeof(aux_gpu));
		cudaMemcpy(aux_gpu, &aux, sizeof(aux), cudaMemcpyHostToDevice);
		*/
		change_gpu chgpu(g);
		output_type_gpu outgpu(out);
		quasi_newton_aux aux_gpu = aux;
		fl res = bfgs(aux_gpu, outgpu.c, chgpu, average_required_improvement, params);
		out.c = outgpu.c.to_conf();
		out.e = res;
	}
	else {
		fl res = bfgs(aux, out.c, g, average_required_improvement, params);
		out.e = res;
	}
}

