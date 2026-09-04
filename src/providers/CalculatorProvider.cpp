#include "CalculatorProvider.h"
#include "../services/ClipboardService.h"
#include <cmath>
#include <cctype>
#include <QStringList>
#include <QRegularExpression>

CalculatorProvider::CalculatorProvider(QObject *parent)
    : SearchProvider(parent)
{
}

namespace {

class MathParser {
public:
    explicit MathParser(const QString &expr) : m_expr(expr), m_pos(0) {}

    std::optional<double> parse() {
        skipWhitespace();
        if (m_pos >= m_expr.length()) return std::nullopt;
        auto val = parseExpression();
        skipWhitespace();
        if (m_pos < m_expr.length()) {
            return std::nullopt; // Extra characters left over
        }
        return val;
    }

private:
    QString m_expr;
    int m_pos;

    void skipWhitespace() {
        while (m_pos < m_expr.length() && m_expr[m_pos].isSpace()) {
            m_pos++;
        }
    }

    QChar peek() {
        skipWhitespace();
        if (m_pos < m_expr.length()) return m_expr[m_pos];
        return QChar();
    }

    QChar get() {
        skipWhitespace();
        if (m_pos < m_expr.length()) return m_expr[m_pos++];
        return QChar();
    }

    std::optional<double> parseExpression() {
        auto left = parseTerm();
        if (!left) return std::nullopt;

        while (true) {
            skipWhitespace();
            QChar c = peek();
            if (c == '+') {
                get();
                auto right = parseTerm();
                if (!right) return std::nullopt;
                *left = *left + *right;
            } else if (c == '-') {
                get();
                auto right = parseTerm();
                if (!right) return std::nullopt;
                *left = *left - *right;
            } else {
                break;
            }
        }
        return left;
    }

    std::optional<double> parseTerm() {
        auto left = parseFactor();
        if (!left) return std::nullopt;

        while (true) {
            skipWhitespace();
            QChar c = peek();
            if (c == '*' || c == QChar(0x00D7) /* × */) {
                get();
                auto right = parseFactor();
                if (!right) return std::nullopt;
                *left = *left * *right;
            } else if (c == '/' || c == QChar(0x00F7) /* ÷ */) {
                get();
                auto right = parseFactor();
                if (!right || *right == 0.0) return std::nullopt; // Div by zero
                *left = *left / *right;
            } else if (c == '%') {
                get();
                auto right = parseFactor();
                if (!right || *right == 0.0) return std::nullopt;
                *left = std::fmod(*left, *right);
            } else {
                break;
            }
        }
        return left;
    }

    std::optional<double> parseFactor() {
        auto left = parsePower();
        if (!left) return std::nullopt;

        skipWhitespace();
        if (peek() == '^') {
            get();
            auto right = parseFactor(); // Right-associative
            if (!right) return std::nullopt;
            *left = std::pow(*left, *right);
        }
        return left;
    }

    std::optional<double> parsePower() {
        skipWhitespace();
        QChar c = peek();
        if (c == '+') {
            get();
            return parsePower();
        } else if (c == '-') {
            get();
            auto val = parsePower();
            if (val) return -(*val);
            return std::nullopt;
        }

        return parsePrimary();
    }

    std::optional<double> parsePrimary() {
        skipWhitespace();
        QChar c = peek();

        if (c == '(') {
            get();
            auto val = parseExpression();
            if (!val) return std::nullopt;
            skipWhitespace();
            if (get() != ')') return std::nullopt;
            return val;
        }

        if (c.isDigit() || c == '.') {
            int start = m_pos;
            bool hasDot = false;
            while (m_pos < m_expr.length() && (m_expr[m_pos].isDigit() || m_expr[m_pos] == '.')) {
                if (m_expr[m_pos] == '.') {
                    if (hasDot) return std::nullopt; // Multiple dots
                    hasDot = true;
                }
                m_pos++;
            }
            bool ok = false;
            double val = m_expr.mid(start, m_pos - start).toDouble(&ok);
            if (!ok) return std::nullopt;
            return val;
        }

        if (c.isLetter()) {
            int start = m_pos;
            while (m_pos < m_expr.length() && m_expr[m_pos].isLetter()) {
                m_pos++;
            }
            QString name = m_expr.mid(start, m_pos - start).toLower();

            // Constants
            if (name == "pi") return M_PI;
            if (name == "e") return M_E;

            // Functions
            skipWhitespace();
            if (peek() == '(') {
                get();
                auto arg = parseExpression();
                if (!arg) return std::nullopt;
                skipWhitespace();
                if (get() != ')') return std::nullopt;

                if (name == "sqrt") {
                    if (*arg < 0.0) return std::nullopt;
                    return std::sqrt(*arg);
                } else if (name == "sin") return std::sin(*arg);
                else if (name == "cos") return std::cos(*arg);
                else if (name == "tan") return std::tan(*arg);
                else if (name == "abs") return std::abs(*arg);
                else if (name == "ln") {
                    if (*arg <= 0.0) return std::nullopt;
                    return std::log(*arg);
                } else if (name == "log") {
                    if (*arg <= 0.0) return std::nullopt;
                    return std::log10(*arg);
                } else if (name == "exp") return std::exp(*arg);
                else if (name == "round") return std::round(*arg);
                else if (name == "floor") return std::floor(*arg);
                else if (name == "ceil") return std::ceil(*arg);
            }
        }

        return std::nullopt;
    }
};

} // namespace

bool CalculatorProvider::isMathExpression(const QString &query)
{
    // Must contain at least one digit and one math operator, or function call
    static QRegularExpression opRegex("[+\\-*/%^×÷]");
    static QRegularExpression funcRegex("\\b(sqrt|sin|cos|tan|log|ln|abs|exp)\\s*\\(");
    static QRegularExpression digitRegex("\\d");

    bool hasDigit = query.contains(digitRegex);
    bool hasOp = query.contains(opRegex);
    bool hasFunc = query.contains(funcRegex);

    return (hasDigit && hasOp) || hasFunc;
}

std::optional<double> CalculatorProvider::evaluate(const QString &expression)
{
    MathParser parser(expression);
    return parser.parse();
}

QList<SearchResult> CalculatorProvider::search(const QString &query)
{
    QList<SearchResult> results;
    QString q = query.trimmed();

    if (!isMathExpression(q))
        return results;

    auto val = evaluate(q);
    if (!val || std::isnan(*val) || std::isinf(*val))
        return results;

    double res = *val;
    QString formatted;
    if (std::abs(res - std::round(res)) < 1e-9 && std::abs(res) < 1e15) {
        formatted = QString::number(static_cast<qint64>(std::round(res)));
    } else {
        formatted = QString::number(res, 'g', 10);
    }

    SearchResult sr;
    sr.setId("calc:" + formatted);
    sr.setTitle(formatted);
    sr.setSubtitle(q + " = " + formatted + " · Press Enter to copy");
    sr.setIcon("accessories-calculator");
    sr.setScore(100.0);
    sr.setType("Calculator");
    sr.setProvider("Calculator");
    sr.setAction("copy");
    sr.setMetadataValue("result", formatted);
    sr.setSecondaryActionLabel("Copy Equation");
    sr.setSecondaryAction("copy_equation");
    sr.setMetadataValue("equation", q + " = " + formatted);

    results.append(sr);
    return results;
}

bool CalculatorProvider::execute(const SearchResult &result, const QString &action)
{
    if (action == "copy_equation") {
        QString eq = result.metadataValue("equation").toString();
        ClipboardService::instance().setText(eq);
        return true;
    }

    QString text = result.metadataValue("result").toString();
    if (!text.isEmpty()) {
        ClipboardService::instance().setText(text);
        return true;
    }
    return false;
}
