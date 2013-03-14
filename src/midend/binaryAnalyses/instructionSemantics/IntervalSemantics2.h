#ifndef Rose_IntervalSemantics_H
#define Rose_IntervalSemantics_H
#include <stdint.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include "BaseSemantics2.h"
#include "integerOps.h"
#include "rangemap.h"

namespace BinaryAnalysis {              // documented elsewhere
namespace InstructionSemantics2 {       // documented elsewhere


/** An interval analysis semantic domain.
 *
 *  Each value in this domain is a set of intervals in the 32-bit unsigned integer space.  The intervals are represented by
 *  ROSE's Range type and the set of ranges is represented by ROSE's RangeMap class. In other words, a semantic value is
 *  actually a discontiguous set of intervals rather than the single interval that's usually used in these kinds of analyses. */
namespace IntervalSemantics {

/** Range of possible values.  We only define this so the range-printing methods are a bit more intuitive for semantic
 *  analysis.  Otherwise we'll end up using the Extent::print() method which is more suitable for things like section
 *  addresses. */
class Interval: public Range<uint64_t> {
public:
    Interval(): Range<uint64_t>() {}
    explicit Interval(uint64_t first): Range<uint64_t>(first) {}
    Interval(uint64_t first, uint64_t size): Range<uint64_t>(first, size) {}
    Interval(const Range<uint64_t> &other): Range<uint64_t>(other) {} /*implicit*/

    static Interval inin(uint64_t first, uint64_t last) /*override*/ {
        assert(first<=last);
        Interval retval;
        retval.first(first);
        retval.last(last);
        return retval;
    }

    /** Convert a bit mask to a string. */
    static std::string to_string(uint64_t n);

    void print(std::ostream &o) const /*override*/;
};

std::ostream& operator<<(std::ostream &o, const Interval &x);

/** Set of intervals. */
typedef RangeMap<Interval> Intervals;

/*******************************************************************************************************************************
 *                                      Semantic value
 *******************************************************************************************************************************/

/** Smart pointer to an SValue object. SValue objects are reference counted and should not be explicitly deleted. */
typedef BaseSemantics::Pointer<class SValue> SValuePtr;

/** Type of values manipulated by the IntervalSemantics domain. */
class SValue: public BaseSemantics::SValue {
protected:
    Intervals p_intervals;

protected:
    // Protected constructors. See base class and public members for documentation
    explicit SValue(size_t nbits): BaseSemantics::SValue(nbits) {
        p_intervals.insert(Interval::inin(0, IntegerOps::genMask<uint64_t>(nbits)));
    }
    SValue(size_t nbits, uint64_t number): BaseSemantics::SValue(nbits) {
        number &= IntegerOps::genMask<uint64_t>(nbits);
        p_intervals.insert(Interval(number));
    }
    SValue(size_t nbits, uint64_t v1, uint64_t v2): BaseSemantics::SValue(nbits) {
        v1 &= IntegerOps::genMask<uint64_t>(nbits);
        v2 &= IntegerOps::genMask<uint64_t>(nbits);
        assert(v1<=v2);
        p_intervals.insert(Interval::inin(v1, v2));
    }
    SValue(size_t nbits, const Intervals &intervals): BaseSemantics::SValue(nbits) {
        assert(!intervals.empty());
        assert((intervals.max() <= IntegerOps::genMask<uint64_t>(nbits)));
        p_intervals = intervals;
    }
        

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Static allocating constructors
public:
    /** Construct a prototypical value. Prototypical values are only used for their virtual constructors. */
    static SValuePtr instance() {
        return SValuePtr(new SValue(1));
    }

    /** Create a value from a set of intervals. */
    static SValuePtr instance(size_t nbits, const Intervals &intervals) {
        return SValuePtr(new SValue(nbits, intervals));
    }

    /** Create a value from a set of possible bits. */
    static SValuePtr instance_from_bits(size_t nbits, uint64_t possible_bits);

    /** Promote a base value to an IntevalSemantics value. The value @p v must have an IntervalSemantics::SValue dynamic type. */
    static SValuePtr promote(const BaseSemantics::SValuePtr &v) { // hot
        SValuePtr retval = BaseSemantics::dynamic_pointer_cast<SValue>(v);
        assert(retval!=NULL);
        return retval;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Virtual allocating constructors
public:
    virtual BaseSemantics::SValuePtr undefined_(size_t nbits) const /*override*/ {
        return SValuePtr(new SValue(nbits));
    }

    virtual BaseSemantics::SValuePtr number_(size_t nbits, uint64_t number) const /*override*/ {
        return SValuePtr(new SValue(nbits, number));
    }
    virtual BaseSemantics::SValuePtr copy(size_t new_width=0) const /*override*/ {
        SValuePtr retval(new SValue(*this));
        if (new_width!=0 && new_width!=retval->get_width())
            retval->set_width(new_width);
        return retval;
    }
    
    /** Construct a ValueType that's constrained to be between two unsigned values, inclusive. */
    virtual SValuePtr create(size_t nbits, uint64_t v1, uint64_t v2) {
        return SValuePtr(new SValue(nbits, v1, v2));
    }

    /** Construct a ValueType from a rangemap. Note that this does not truncate the rangemap to contain only values that would
     *  be possible for the ValueType size--see unsignedExtend() for that. */
    virtual SValuePtr create(size_t nbits, const Intervals &intervals) {
        return instance(nbits, intervals); 
    }

    /** Generate ranges from bits. Given the set of bits that could be set, generate a range.  We have to be careful here
     *  because we could end up generating very large rangemaps: a rangemap where the high 31 bits could be set but the zero
     *  bit must be cleared would create a rangemap with 2^31 singleton entries. */
    virtual SValuePtr create_from_bits(size_t nbits, uint64_t possible_bits) {
        return instance_from_bits(nbits, possible_bits);
    }
            

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Override virtual methods...
public:
    virtual bool may_equal(const BaseSemantics::SValuePtr &other, SMTSolver *solver=NULL) const /*override*/;
    virtual bool must_equal(const BaseSemantics::SValuePtr &other, SMTSolver *solver=NULL) const /*override*/;

    virtual bool is_number() const /*override*/ {
        return 1==p_intervals.size();
    }
    
    virtual uint64_t get_number() const {
        assert(1==p_intervals.size());
        return p_intervals.min();
    }

    virtual void print(std::ostream &output, BaseSemantics::PrintHelper *helper=NULL) const /*override*/ {
        output <<p_intervals <<"[" <<get_width() <<"]";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Additional methods introduced at this level of the class hierarchy
public:
    /** Returns the rangemap stored in this value. */
    const Intervals& get_intervals() const {
        return p_intervals;
    }

    /** Changes the rangemap stored in the value. */
    void set_intervals(const Intervals &intervals) {
        p_intervals = intervals;
    }

    /** Returns all possible bits that could be set. */
    uint64_t possible_bits() const;

};

/*******************************************************************************************************************************
 *                                      Memory State
 *******************************************************************************************************************************/

/** Smart pointer to a MemoryState object.  MemoryState objects are reference counted and should not be explicitly deleted. */
typedef boost::shared_ptr<class MemoryState> MemoryStatePtr;

/** Byte-addressable memory.
 *
 *  This class represents an entire state of memory via a list of memory cells.  The memory cell list is sorted in reverse
 *  chronological order and addresses that satisfy a "must-alias" predicate are pruned so that only the must recent such memory
 *  cell is in the table.
 *
 *  A memory write operation prunes away any existing memory cell that must-alias the newly written address, then adds a new
 *  memory cell to the front of the memory cell list.
 *
 *  A memory read operation scans the memory cell list and returns the union of all possible matches. */
class MemoryState: public BaseSemantics::MemoryCellList {
protected:
    // protected constructors. See base class and public constructors for documentation
    MemoryState(const BaseSemantics::MemoryCellPtr &protocell, const BaseSemantics::SValuePtr &protoval)
        : BaseSemantics::MemoryCellList(protocell, protoval) {}

public:
    /** Static allocating constructor.  This constructor uses BaseSemantics::MemoryCell as the cell type. */
    static  MemoryStatePtr instance(const BaseSemantics::SValuePtr &protoval) {
        BaseSemantics::MemoryCellPtr protocell = BaseSemantics::MemoryCell::instance(protoval, protoval);
        return MemoryStatePtr(new MemoryState(protocell, protoval));
    }

    /** Static allocating constructor. */
    static MemoryStatePtr instance(const BaseSemantics::MemoryCellPtr &protocell, const BaseSemantics::SValuePtr &protoval) {
        return MemoryStatePtr(new MemoryState(protocell, protoval));
    }

    virtual BaseSemantics::MemoryStatePtr create(const BaseSemantics::SValuePtr &protoval) const /*override*/ {
        return instance(protoval);
    }

    virtual BaseSemantics::MemoryStatePtr create(const BaseSemantics::MemoryCellPtr &protocell,
                                                 const BaseSemantics::SValuePtr &protoval) const /*override*/ {
        return instance(protocell, protoval);
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods that override the base class
public:
    /** Read a byte from memory.
     *
     *  In order to read a multi-byte value, use RiscOperators::readMemory(). */
    virtual BaseSemantics::SValuePtr readMemory(const BaseSemantics::SValuePtr &addr, const BaseSemantics::SValuePtr &dflt,
                                                size_t nbits, BaseSemantics::RiscOperators *ops) /*override*/;

    /** Write a byte to memory.
     *
     *  In order to write a multi-byte value, use RiscOperators::writeMemory(). */
    virtual void writeMemory(const BaseSemantics::SValuePtr &addr, const BaseSemantics::SValuePtr &value,
                             BaseSemantics::RiscOperators *ops) /*override*/;
};

/*******************************************************************************************************************************
 *                                      RISC Operators
 *******************************************************************************************************************************/

/** Smart pointer to a RiscOperators object.  RiscOperators objects are reference counted and should not be explicitly
 *  deleted. */
typedef boost::shared_ptr<class RiscOperators> RiscOperatorsPtr;

/** RISC operators for interval domains. */
class RiscOperators: public BaseSemantics::RiscOperators {
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Protected constructors
protected:
    explicit RiscOperators(const BaseSemantics::SValuePtr &protoval, SMTSolver *solver=NULL)
        : BaseSemantics::RiscOperators(protoval, solver) {
        (void) SValue::promote(protoval); // make sure its dynamic type is an IntervalSemantics::SValue or subclass thereof
    }

    explicit RiscOperators(const BaseSemantics::StatePtr &state, SMTSolver *solver=NULL)
        : BaseSemantics::RiscOperators(state, solver) {
        (void) SValue::promote(state->get_protoval()); // dynamic type must be IntervalSemantics::SValue or subclass thereof
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Public allocating constructors
public:
    /** Static allocating constructor.  Creates a new RiscOperators object and configures it to use semantic values and states
     *  that are defaults for IntervalSemantics. */
    static RiscOperatorsPtr instance(SMTSolver *solver=NULL) {
        BaseSemantics::SValuePtr protoval = SValue::instance();
        // FIXME: register state shoudl probably be chosen based on an architecture [Robb Matzke 2013-03-07]
        BaseSemantics::RegisterStatePtr registers = BaseSemantics::RegisterStateX86::instance(protoval);
        BaseSemantics::MemoryStatePtr memory = MemoryState::instance(protoval);
        BaseSemantics::StatePtr state = BaseSemantics::State::instance(registers, memory);
        return RiscOperatorsPtr(new RiscOperators(state, solver));
    }

    /** Static allocating constructor.  An SMT solver may be specified as the second argument for convenience. See set_solver()
     *  for details. */
    static RiscOperatorsPtr instance(const BaseSemantics::SValuePtr &protoval, SMTSolver *solver=NULL) {
        return RiscOperatorsPtr(new RiscOperators(protoval, solver));
    }

    /** Static allocating constructor. An SMT solver may be specified as the second argument for convenience. See set_solver()
     *  for details. */
    static RiscOperatorsPtr instance(const BaseSemantics::StatePtr &state, SMTSolver *solver=NULL) {
        return RiscOperatorsPtr(new RiscOperators(state, solver));
    }

    /** Virtual allocating constructor. */
    virtual BaseSemantics::RiscOperatorsPtr create(const BaseSemantics::SValuePtr &protoval,
                                                   SMTSolver *solver=NULL) const /*override*/ {
        return instance(protoval, solver);
    }

    /** Virtual allocating constructor. */
    virtual BaseSemantics::RiscOperatorsPtr create(const BaseSemantics::StatePtr &state,
                                                   SMTSolver *solver=NULL) const /*override*/ {
        return instance(state, solver);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Methods first introduced at this level of the class hierarchy.
public:

    /** Create a new SValue from a set of possible bits.  This is just a convience function so that we don't have to
     *  see so many dynamic casts in the source code. */
    virtual SValuePtr svalue_from_bits(size_t nbits, uint64_t possible_bits) {
        return SValue::promote(protoval)->create_from_bits(nbits, possible_bits);
    }

    /** Create a new SValue from a set of intervals.  This is just a convience function so that we don't have to
     *  see so many dynamic casts in the source code. */
    virtual SValuePtr svalue_from_intervals(size_t nbits, const Intervals &intervals) {
        return SValue::promote(protoval)->create(nbits, intervals);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Override some non-virtual functions only to change their return type for the convenience of us not having to constantly
    // explicitly dynamic cast them to our own SValuePtr type.
    SValuePtr number_(size_t nbits, uint64_t value) {
        return SValue::promote(protoval->number_(nbits, value));
    }
    SValuePtr true_() {
        return SValue::promote(protoval->true_());
    }
    SValuePtr false_() {
        return SValue::promote(protoval->false_());
    }
    SValuePtr undefined_(size_t nbits) {
        return SValue::promote(protoval->undefined_(nbits));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Override methods from base class.  These are the RISC operators that are invoked by a Dispatcher.
public:
    virtual void interrupt(uint8_t inum) /*override*/;
    virtual void sysenter() /*override*/;
    virtual BaseSemantics::SValuePtr and_(const BaseSemantics::SValuePtr &a_,
                                          const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr or_(const BaseSemantics::SValuePtr &a_,
                                         const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr xor_(const BaseSemantics::SValuePtr &a_,
                                          const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr invert(const BaseSemantics::SValuePtr &a_) /*override*/;
    virtual BaseSemantics::SValuePtr extract(const BaseSemantics::SValuePtr &a_,
                                             size_t begin_bit, size_t end_bit) /*override*/;
    virtual BaseSemantics::SValuePtr concat(const BaseSemantics::SValuePtr &a_,
                                            const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr leastSignificantSetBit(const BaseSemantics::SValuePtr &a_) /*override*/;
    virtual BaseSemantics::SValuePtr mostSignificantSetBit(const BaseSemantics::SValuePtr &a_) /*override*/;
    virtual BaseSemantics::SValuePtr rotateLeft(const BaseSemantics::SValuePtr &a_,
                                                const BaseSemantics::SValuePtr &sa_) /*override*/;
    virtual BaseSemantics::SValuePtr rotateRight(const BaseSemantics::SValuePtr &a_,
                                                 const BaseSemantics::SValuePtr &sa_) /*override*/;
    virtual BaseSemantics::SValuePtr shiftLeft(const BaseSemantics::SValuePtr &a_,
                                               const BaseSemantics::SValuePtr &sa_) /*override*/;
    virtual BaseSemantics::SValuePtr shiftRight(const BaseSemantics::SValuePtr &a_,
                                                const BaseSemantics::SValuePtr &sa_) /*override*/;
    virtual BaseSemantics::SValuePtr shiftRightArithmetic(const BaseSemantics::SValuePtr &a_,
                                                          const BaseSemantics::SValuePtr &sa_) /*override*/;
    virtual BaseSemantics::SValuePtr equalToZero(const BaseSemantics::SValuePtr &a_) /*override*/;
    virtual BaseSemantics::SValuePtr ite(const BaseSemantics::SValuePtr &sel_,
                                         const BaseSemantics::SValuePtr &a_,
                                         const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr unsignedExtend(const BaseSemantics::SValuePtr &a_, size_t new_width) /*override*/;
    virtual BaseSemantics::SValuePtr signExtend(const BaseSemantics::SValuePtr &a_, size_t new_width) /*override*/;
    virtual BaseSemantics::SValuePtr add(const BaseSemantics::SValuePtr &a_,
                                         const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr addWithCarries(const BaseSemantics::SValuePtr &a_,
                                                    const BaseSemantics::SValuePtr &b_,
                                                    const BaseSemantics::SValuePtr &c_,
                                                    BaseSemantics::SValuePtr &carry_out/*out*/);
    virtual BaseSemantics::SValuePtr negate(const BaseSemantics::SValuePtr &a_) /*override*/;
    virtual BaseSemantics::SValuePtr signedDivide(const BaseSemantics::SValuePtr &a_,
                                                  const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr signedModulo(const BaseSemantics::SValuePtr &a_,
                                                  const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr signedMultiply(const BaseSemantics::SValuePtr &a_,
                                                    const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr unsignedDivide(const BaseSemantics::SValuePtr &a_,
                                                    const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr unsignedModulo(const BaseSemantics::SValuePtr &a_,
                                                    const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr unsignedMultiply(const BaseSemantics::SValuePtr &a_,
                                                      const BaseSemantics::SValuePtr &b_) /*override*/;
    virtual BaseSemantics::SValuePtr readMemory(X86SegmentRegister sg,
                                                const BaseSemantics::SValuePtr &addr,
                                                const BaseSemantics::SValuePtr &cond,
                                                size_t nbits) /*override*/;
    virtual void writeMemory(X86SegmentRegister sg,
                             const BaseSemantics::SValuePtr &addr,
                             const BaseSemantics::SValuePtr &data,
                             const BaseSemantics::SValuePtr &cond) /*override*/;
};
} /*namespace*/
} /*namespace*/
} /*namespace*/

#endif
