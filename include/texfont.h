/**
 * @file
 * @brief Header for the Custom TeX-Like Physics/Calculus Font.
 *
 * Mapped to the custom "TexFonts.8xv" and "TexScript.8xv" font packs.
 * Range 0x20-0x7F: Standard ASCII.
 * Range 0x80-0xBC: Custom Math Symbols.
 *
 * Generated to match the Python font generation scripts.
 */

#ifndef TEXFONT_H
#define TEXFONT_H

/* ========================================================================= */
/*                        LOWER MATH SYMBOLS (0x00 - 0x1F)                   */
/* ========================================================================= */

#define TEXFONT_CURSOR_BLANK_CHAR '\000'

#define TEXFONT_UNION "\001"
#define TEXFONT_UNION_CHAR '\001'
#define TEXFONT_INTERSECTION "\002"
#define TEXFONT_INTERSECTION_CHAR '\002'
#define TEXFONT_NOT_IN "\003"
#define TEXFONT_NOT_IN_CHAR '\003'
#define TEXFONT_EMPTY_SET "\004"
#define TEXFONT_EMPTY_SET_CHAR '\004'
#define TEXFONT_FOR_ALL "\005"
#define TEXFONT_FOR_ALL_CHAR '\005'
#define TEXFONT_EXISTS "\006"
#define TEXFONT_EXISTS_CHAR '\006'
#define TEXFONT_SUBSET_EQ "\007"
#define TEXFONT_SUBSET_EQ_CHAR '\007'
#define TEXFONT_EQUIVALENT "\010"
#define TEXFONT_EQUIVALENT_CHAR '\010'
#define TEXFONT_SIMILAR "\011"
#define TEXFONT_SIMILAR_CHAR '\011'
#define TEXFONT_CONGRUENT "\012"
#define TEXFONT_CONGRUENT_CHAR '\012'
#define TEXFONT_PROPORTIONAL "\013"
#define TEXFONT_PROPORTIONAL_CHAR '\013'
#define TEXFONT_PERPENDICULAR "\014"
#define TEXFONT_PERPENDICULAR_CHAR '\014'
#define TEXFONT_PARALLEL "\015"
#define TEXFONT_PARALLEL_CHAR '\015'
#define TEXFONT_ANGLE "\016"
#define TEXFONT_ANGLE_CHAR '\016'
#define TEXFONT_RING_OP "\017"
#define TEXFONT_RING_OP_CHAR '\017'
#define TEXFONT_O_PLUS "\020"
#define TEXFONT_O_PLUS_CHAR '\020'

/* Note: Indices 0x11 through 0x1F are currently unused/empty */

/* ========================================================================= */
/*                          CUSTOM MATH SYMBOLS (0x80+)                      */
/* ========================================================================= */

/* --- Greek Lowercase --- */
#define TEXFONT_alpha "\200"
#define TEXFONT_alpha_CHAR '\200'
#define TEXFONT_beta "\201"
#define TEXFONT_beta_CHAR '\201'
#define TEXFONT_gamma "\202"
#define TEXFONT_gamma_CHAR '\202'
#define TEXFONT_delta "\203"
#define TEXFONT_delta_CHAR '\203'
#define TEXFONT_epsilon "\204"
#define TEXFONT_epsilon_CHAR '\204'
#define TEXFONT_zeta "\205"
#define TEXFONT_zeta_CHAR '\205'
#define TEXFONT_eta "\206"
#define TEXFONT_eta_CHAR '\206'
#define TEXFONT_theta "\207"
#define TEXFONT_theta_CHAR '\207'
#define TEXFONT_iota "\210"
#define TEXFONT_iota_CHAR '\210'
#define TEXFONT_kappa "\211"
#define TEXFONT_kappa_CHAR '\211'
#define TEXFONT_lambda "\212"
#define TEXFONT_lambda_CHAR '\212'
#define TEXFONT_mu "\213"
#define TEXFONT_mu_CHAR '\213'
#define TEXFONT_nu "\214"
#define TEXFONT_nu_CHAR '\214'
#define TEXFONT_xi "\215"
#define TEXFONT_xi_CHAR '\215'
#define TEXFONT_omicron "\216"
#define TEXFONT_omicron_CHAR '\216'
#define TEXFONT_pi "\217"
#define TEXFONT_pi_CHAR '\217'
#define TEXFONT_rho "\220"
#define TEXFONT_rho_CHAR '\220'
#define TEXFONT_sigma "\221"
#define TEXFONT_sigma_CHAR '\221'
#define TEXFONT_tau "\222"
#define TEXFONT_tau_CHAR '\222'
#define TEXFONT_upsilon "\223"
#define TEXFONT_upsilon_CHAR '\223'
#define TEXFONT_phi "\224"
#define TEXFONT_phi_CHAR '\224'
#define TEXFONT_chi "\225"
#define TEXFONT_chi_CHAR '\225'
#define TEXFONT_psi "\226"
#define TEXFONT_psi_CHAR '\226'
#define TEXFONT_omega "\227"
#define TEXFONT_omega_CHAR '\227'

/* --- Greek Uppercase --- */
#define TEXFONT_GAMMA "\230"
#define TEXFONT_GAMMA_CHAR '\230'
#define TEXFONT_DELTA "\231"
#define TEXFONT_DELTA_CHAR '\231'
#define TEXFONT_THETA "\232"
#define TEXFONT_THETA_CHAR '\232'
#define TEXFONT_LAMBDA "\233"
#define TEXFONT_LAMBDA_CHAR '\233'
#define TEXFONT_XI "\234"
#define TEXFONT_XI_CHAR '\234'
#define TEXFONT_PI "\235"
#define TEXFONT_PI_CHAR '\235'
#define TEXFONT_SIGMA "\236"
#define TEXFONT_SIGMA_CHAR '\236'
#define TEXFONT_PHI "\237"
#define TEXFONT_PHI_CHAR '\237'
#define TEXFONT_PSI "\240"
#define TEXFONT_PSI_CHAR '\240'
#define TEXFONT_OMEGA "\241"
#define TEXFONT_OMEGA_CHAR '\241'

/* --- Calculus --- */
#define TEXFONT_PARTIAL "\242"
#define TEXFONT_PARTIAL_CHAR '\242'
#define TEXFONT_INFINITY "\243"
#define TEXFONT_INFINITY_CHAR '\243'
#define TEXFONT_NABLA "\244"
#define TEXFONT_NABLA_CHAR '\244'
#define TEXFONT_PRIME "\245"
#define TEXFONT_PRIME_CHAR '\245'
#define TEXFONT_ELL "\246"
#define TEXFONT_ELL_CHAR '\246'
#define TEXFONT_HBAR "\247"
#define TEXFONT_HBAR_CHAR '\247'
#define TEXFONT_DEGREE "\250"
#define TEXFONT_DEGREE_CHAR '\250'

/* --- Big Operators --- */
#define TEXFONT_INTEGRAL "\251"
#define TEXFONT_INTEGRAL_CHAR '\251'
#define TEXFONT_SUMMATION "\252"
#define TEXFONT_SUMMATION_CHAR '\252'
#define TEXFONT_PRODUCT "\253"
#define TEXFONT_PRODUCT_CHAR '\253'

/* --- Operators --- */
#define TEXFONT_PLUS_MINUS "\254"
#define TEXFONT_PLUS_MINUS_CHAR '\254'
#define TEXFONT_MINUS_PLUS "\255"
#define TEXFONT_MINUS_PLUS_CHAR '\255'
#define TEXFONT_DOT_OP "\256"
#define TEXFONT_DOT_OP_CHAR '\256'
#define TEXFONT_TIMES "\257"
#define TEXFONT_TIMES_CHAR '\257'
#define TEXFONT_DIVIDE "\260"
#define TEXFONT_DIVIDE_CHAR '\260'
#define TEXFONT_APPROX "\261"
#define TEXFONT_APPROX_CHAR '\261'
#define TEXFONT_NOT_EQUAL "\262"
#define TEXFONT_NOT_EQUAL_CHAR '\262'
#define TEXFONT_LESS_EQUAL "\263"
#define TEXFONT_LESS_EQUAL_CHAR '\263'
#define TEXFONT_GREATER_EQUAL "\264"
#define TEXFONT_GREATER_EQUAL_CHAR '\264'

/* --- Arrows --- */
#define TEXFONT_ARROW_RIGHT "\265"
#define TEXFONT_ARROW_RIGHT_CHAR '\265'
#define TEXFONT_ARROW_LEFT "\266"
#define TEXFONT_ARROW_LEFT_CHAR '\266'

/* --- Logic & Sets --- */
#define TEXFONT_ELEMENT_OF "\267"
#define TEXFONT_ELEMENT_OF_CHAR '\267'
#define TEXFONT_SUBSET_OF "\270"
#define TEXFONT_SUBSET_OF_CHAR '\270'
#define TEXFONT_THEREFORE "\271"
#define TEXFONT_THEREFORE_CHAR '\271'
#define TEXFONT_LANGLE "\272"
#define TEXFONT_LANGLE_CHAR '\272'
#define TEXFONT_RANGLE "\273"
#define TEXFONT_RANGLE_CHAR '\273'

/* --- Structural --- */
#define TEXFONT_SQRT_HEAD "\274"
#define TEXFONT_SQRT_HEAD_CHAR '\274'

#endif /* TEXFONT_H */
