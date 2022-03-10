#ifndef MEM_POOL_UTILS_DETAIL
#define MEM_POOL_UTILS_DETAIL

typedef unsigned int static_gcd_val;

namespace detail {

#define STATIC_CONST(type, exp) static constexpr type exp

	//Euclid method to find  greatest common divisor(GCD)
	template< static_gcd_val value1, static_gcd_val value2>
	struct static_gcd_t
	{
	private:
		STATIC_CONST(static_gcd_val, newValue1 = value2);
		STATIC_CONST(static_gcd_val, newValue2 = value1 % value2);

		typedef static_gcd_t<newValue1, newValue2> nextStep;
	public:
		STATIC_CONST(static_gcd_val, value = nextStep::value);
	};

	//specialization
	template <static_gcd_val value1>
	struct static_gcd_t<value1, 0u>
	{
	public:
		STATIC_CONST(static_gcd_val, value = value1);
	};

	//the least common multiple(LCM) from the GCD
	template< static_gcd_val value1, static_gcd_val value2>
	struct static_lcm_t
	{
	private:
		typedef static_gcd_t<value1, value2> gcd;
	public:
		STATIC_CONST(static_gcd_val, value = value1 * value2 / gcd::value);
	};

	//specialization
	template<>
	struct static_lcm_t<0u, 0u>
	{
		STATIC_CONST(static_gcd_val, value = 0u);
	};

}//detail

//aliases
template<static_gcd_val value1, static_gcd_val value2>
struct static_gcd
{
public:
	STATIC_CONST(static_gcd_val, value = (detail::static_gcd_t<value1, value2>::value));
};

template <static_gcd_val value1, static_gcd_val value2>
struct static_lcm
{
public:
	STATIC_CONST(static_gcd_val, value = (detail::static_lcm_t<value1, value2>::value));
};

#undef STATIC_CONST
#endif //MEM_POOL_UTILS_DETAIL