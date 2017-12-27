/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_RVSDG_CONTROLTYPE_H
#define JIVE_RVSDG_CONTROLTYPE_H

#include <jive/rvsdg/type.h>

namespace jive {
namespace ctl {

class type final : public jive::statetype {
public:
	virtual ~type() noexcept;

	type(size_t nalternatives);

	virtual std::string debug_string() const override;

	virtual bool operator==(const jive::type & other) const noexcept override;

	virtual std::unique_ptr<jive::type>
	copy() const override;

	inline size_t
	nalternatives() const noexcept
	{
		return nalternatives_;
	}

private:
	size_t nalternatives_;
};

}

static inline bool
is_ctltype(const jive::type & type) noexcept
{
	return dynamic_cast<const jive::ctl::type*>(&type) != nullptr;
}

namespace ctl {

class value_repr {
public:
	value_repr(size_t alternative, size_t nalternatives);

	inline bool
	operator==(const jive::ctl::value_repr & other) const noexcept
	{
		return alternative_ == other.alternative_ && nalternatives_ == other.nalternatives_;
	}

	inline bool
	operator!=(const jive::ctl::value_repr & other) const noexcept
	{
		return !(*this == other);
	}

	inline size_t
	alternative() const noexcept
	{
		return alternative_;
	}

	inline size_t
	nalternatives() const noexcept
	{
		return nalternatives_;
	}

private:
	size_t alternative_;
	size_t nalternatives_;
};

const type boolean(2);

}
}

#endif