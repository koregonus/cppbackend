#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const ShowBookInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace detail

std::ostream& operator<<(std::ostream& out, const ShowBookInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const show_single_book_t& book) {
    out << "Title: " << book.title << std::endl;
    out << "Author: " << book.author_name << std::endl;
    out << "Publication year: " << book.publication_year << std::endl;
    if(book.tags.size() > 0)
    {
        out << "Tags: ";
        for(int i = 0; i < book.tags.size(); i++)
        {
            out << book.tags[i];
            if((i+1) < book.tags.size())
            {
                out << ", ";
            }
        }
        out << std::endl;
    }
    return out;
}


template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowBook"s, {}, "Show single book"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteAuthor"s, {}, "Delete author and his books"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, {}, "Delete book and its tags"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditAuthor"s, {}, "Edit author name"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("EditBook"s, {}, "Edit book data"s, std::bind(&View::EditBook, this, ph::_1));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if(name.size() == 0)
            throw std::exception();
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            use_cases_.SaveBook((*params).title, (*params).author_id,
                                (*params).publication_year, (*params).tags);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}


bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::vector<std::string> View::EnterTags() const
{
    std::vector<std::string> ret;
    output_ << "Enter tags (comma separated):" << std::endl;
    std::string str;
    ret = std::vector<std::string>{};
    if (!std::getline(input_, str) || str.empty()) {
        // return ret;
    }
    else
    {
        ret = std::vector<std::string>{};
        std::istringstream iss(str);
        std::string tag_item;
        while (std::getline(iss, tag_item, ',')) {
            boost::algorithm::trim(tag_item);
            auto it = std::unique(tag_item.begin(), tag_item.end(),
                [](char const &lhs, char const &rhs) {
                    return (lhs == rhs) && (lhs == ' ');
                });
     
            tag_item.erase(it, tag_item.end());

            if (!(std::find((ret).begin(), (ret).end(), tag_item) != (ret).end()))
            {
                if(tag_item.size() == 0)
                    continue;
              // Element in vector.
                (ret).push_back(tag_item);
            }
            if((ret).size() > 0)
            {
                std::sort((ret).begin(), (ret).end());
            }
        }
    }
    
    // if((*ret).size() > 0)
    // {
    //     return ret;
    // }

    return ret;
}

bool View::ShowBook(std::istream& cmd_input) const
{
    std::string str;
    if(!std::getline(cmd_input, str) || str.empty())
    {
        auto book_id = SelectBook();
        if (not book_id.has_value())
        {
            return true;
        }
        else {
            auto result = use_cases_.ShowBook(std::move(book_id.value()));
            output_ << result;
        }
    }
    else
    {
        boost::algorithm::trim(str);
        auto books = GetBooks();
        std::vector<show_book_t> found_books;
        for(auto& item : books)
        {
            if(item.title == str)
            {
                found_books.push_back(item);
            }
        }
        if(found_books.size() == 0)
        {
            return true;
        }
        else if(found_books.size() == 1)
        {
            auto result = use_cases_.ShowBook(std::move(found_books[0].book_id));
            output_ << result;
        }
        else
        {
            PrintVector(output_, found_books);
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string book_collision;
            if(!std::getline(input_, book_collision) || book_collision.empty())
            {
                return true;
            }
            else
            {
                int book_idx;
                try {
                    book_idx = std::stoi(book_collision);
                } catch (std::exception const&) {
                    return true;
                }

                --book_idx;
                if (book_idx < 0 or book_idx >= found_books.size()) {
                    // throw std::runtime_error("Invalid author num");
                    return true;
                }
                auto result = use_cases_.ShowBook(std::move(found_books[book_idx].book_id));
                output_ << result;
            }
        }
    }
    return true;
}

std::string View::FillTitleData(std::string& original_title) const
{
    std::string in_buffer;
    output_ << "Enter new title or empty line to use the current one (" << original_title << ")" << std::endl;
    if(!(!std::getline(input_, in_buffer) || in_buffer.empty()))
    {
        boost::algorithm::trim(in_buffer);
    }
    if(in_buffer.empty())
    {
        in_buffer = original_title;
    }
    return in_buffer;
}

int View::FillPubYearData(int original_pub_year) const
{
    int ret = original_pub_year;
    std::string in_buffer;
    output_ << "Enter publication year or empty line to use the current one (" << original_pub_year << ")" << std::endl;
    if(!(!std::getline(input_, in_buffer) || in_buffer.empty()))
    {
        boost::algorithm::trim(in_buffer);
        if(!in_buffer.empty())
        {
            ret = std::stoi(in_buffer); 
        }
    }
    return ret;
}


std::vector<std::string> View::FillTagsData(std::vector<std::string>& tags) const
{
    std::vector<std::string> ret;
    output_ << "Enter tags (current tags: ";
        for(int i = 0; i < tags.size(); i++)
        {
            output_ << tags[i];
            if(i+1 != tags.size())
            {
                output_ << ", ";
            }
        }
        output_ << "):" << std::endl;
        std::string str;
        if (!(!std::getline(input_, str) || str.empty())) {
            std::istringstream iss(str);
            std::string tag_item;
            while (std::getline(iss, tag_item, ',')) {
                boost::algorithm::trim(tag_item);
                auto it = std::unique(tag_item.begin(), tag_item.end(),
                    [](char const &lhs, char const &rhs) {
                        return (lhs == rhs) && (lhs == ' ');
                    });
         
                tag_item.erase(it, tag_item.end());

                if (!(std::find((ret).begin(), (ret).end(), tag_item) != (ret).end()))
                {
                  // Element in vector.
                    (ret).push_back(tag_item);
                }
            }
            std::sort(ret.begin(), ret.end());
            // if((ret).size() == 0)
            // {
            //      ret = tags;
            // }
        }
        // else
        // {
        //     ret = tags;
        // }
    return ret;
}

bool View::EditBook(std::istream& cmd_input) const
{
    std::string str;
    show_book_t new_book_data;
    if(!std::getline(cmd_input, str) || str.empty())
    {
        auto book_id = SelectBook();
        if (not book_id.has_value())
        {
            output_ << "Book not found" << std::endl;
            return true;
        }
        else {
            auto result = use_cases_.ShowBook(std::move(book_id.value()));
            result.title = FillTitleData(result.title);
            result.publication_year = FillPubYearData(result.publication_year);
            result.tags = FillTagsData(result.tags);
            try
            {
                use_cases_.EditBook(result, *book_id);
            }
            catch(...)
            {
                output_ << "Book not found" << std::endl;
                return true;
            }
        }
    }
    else
    {
        boost::algorithm::trim(str);
        auto books = GetBooks();
        std::vector<show_book_t> found_books;
        for(auto& item : books)
        {
            if(item.title == str)
            {
                found_books.push_back(item);
            }
        }
        if(found_books.size() == 1)
        {
            auto result = use_cases_.ShowBook(found_books[0].book_id);
            result.title = FillTitleData(result.title);
            result.publication_year = FillPubYearData(result.publication_year);
            result.tags = FillTagsData(result.tags);
            try
            {
                use_cases_.EditBook(result, found_books[0].book_id);
            }
            catch(...)
            {
                output_ << "Book not found" << std::endl;
                return true;
            }
        }
        if(found_books.size() == 0)
        {
            output_ << "Book not found" << std::endl;
            return true;
        }
        else
        {
            PrintVector(output_, found_books);
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string book_collision;
            if(!std::getline(input_, book_collision) || book_collision.empty())
            {
                output_ << "Book not found" << std::endl;
                return true;
            }
            else
            {
                int book_idx;
                try {
                    book_idx = std::stoi(book_collision);
                } catch (std::exception const&) {
                    output_ << "Book not found" << std::endl;
                    return true;
                }

                --book_idx;
                if (book_idx < 0 or book_idx >= found_books.size()) {
                    output_ << "Book not found" << std::endl;
                    return true;
                }
                // enter new data
                auto result = use_cases_.ShowBook(found_books[book_idx].book_id);
                result.title = FillTitleData(result.title);
                result.publication_year = FillPubYearData(result.publication_year);
                result.tags = FillTagsData(result.tags);
                try
                {
                    use_cases_.EditBook(result, found_books[book_idx].book_id);
                }
                catch(...)
                {
                    output_ << "Book not found" << std::endl;
                    return true;
                }
            }
        }
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const
{
    std::string str;
    if(!std::getline(cmd_input, str) || str.empty())
    {
        auto book_id = SelectBook();
        if (not book_id.has_value())
        {
            output_ << "Book not found" << std::endl;
            return true;
        }
        else {
            try
            {
                use_cases_.DeleteBook(std::move(book_id.value()));
            }
            catch(...)
            {
                output_ << "Failed to delete book" << std::endl;
            }
        }
    }
    else
    {
        boost::algorithm::trim(str);
        auto books = GetBooks();
        std::vector<show_book_t> found_books;
        for(auto& item : books)
        {
            if(item.title == str)
            {
                found_books.push_back(item);
            }
        }
        if(found_books.size() == 1)
        {
            try
            {
                use_cases_.DeleteBook(std::move(found_books[0].book_id));
            }
            catch(...)
            {
                output_ << "Failed to delete book" << std::endl;
            }
        }
        else if(found_books.size() == 0)
        {
            output_ << "Book not found" << std::endl;
        }
        else
        {
            PrintVector(output_, found_books);
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string book_collision;
            if(!std::getline(input_, book_collision) || book_collision.empty())
            {
                return true;
            }
            else
            {
                int book_idx;
                try {
                    book_idx = std::stoi(book_collision);
                } catch (std::exception const&) {
                    return true;
                }

                --book_idx;
                if (book_idx < 0 or book_idx >= found_books.size()) {
                    output_ << "Failed to delete book" << std::endl;
                    return true;
                }
                try
                {
                    use_cases_.DeleteBook(std::move(found_books[0].book_id));
                }
                catch(...)
                {
                    output_ << "Failed to delete book" << std::endl;
                }
                use_cases_.DeleteBook(std::move(found_books[book_idx].book_id));
                // output_ << result;
            }
        }
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const
{
    std::string str;
    if(!std::getline(cmd_input,str) || str.empty())
    {
        auto author_id = SelectAuthor();
        if (not author_id.has_value())
        {
            output_ << "Failed to delete author" << std::endl;
        }
        else {
            use_cases_.DeleteAuthor(std::move(author_id.value()));
        }
    }
    else
    {
        boost::algorithm::trim(str);
        if(str.size() == 0)
        {
            output_ << "Failed to delete author" << std::endl;
            return true;
        }
        else
        {
            auto authors = GetAuthors();
            for(auto& item : authors)
            {
                if(item.name == str)
                {
                    use_cases_.DeleteAuthor(std::move(item.id));
                    return true;
                }
            }
            output_ << "Failed to delete author" << std::endl;
        }
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const
{
    std::string str;
    if(!std::getline(cmd_input,str) || str.empty())
    {
        auto author_id = SelectAuthor();
        if (not author_id.has_value())
        {
            output_ << "Failed to edit author" << std::endl;
        }
        else {
            try{
                output_ << "Enter new name:" << std::endl;
                std::string new_name;
                if(!std::getline(input_, new_name) || new_name.empty())
                {
                    throw std::exception();
                }
                boost::algorithm::trim(str);
                if(new_name.empty())
                {
                    throw std::exception();   
                }

                use_cases_.EditAuthor(std::move(author_id.value()), std::move(new_name));
            }
            catch(...)
            {
                output_ << "Failed to edit author" << std::endl;
                return true;
            }
        }
    }
    else
    {
        boost::algorithm::trim(str);
        if(str.size() == 0)
        {
            output_ << "Failed to edit author" << std::endl;
            return true;
        }
        else
        {
            auto authors = GetAuthors();
            for(auto& item : authors)
            {
                if(item.name == str)
                {
                    try{
                        std::string new_name;
                        output_ << "Enter new name:" << std::endl;
                        if(!std::getline(input_, new_name) || new_name.empty())
                        {
                            throw std::exception();
                        }
                        boost::algorithm::trim(new_name);

                        if(new_name.empty())
                        {
                            throw std::exception();   
                        }
                        use_cases_.EditAuthor(std::move(item.id), std::move(new_name));
                        return true;
                    }
                    catch(...)
                    {
                        output_ << "Failed to edit author" << std::endl;
                        break;
                    }
                }
            }
            output_ << "Failed to edit author" << std::endl;
        }
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    // std::cout << "entered to GBP\n";

    cmd_input >> params.publication_year;

    if(cmd_input.fail())
    {
        output_ << "Failed to add book" << std::endl;
        // output_ << "cmd" << std::endl;
        return std::nullopt;
    }

    std::getline(cmd_input, params.title);

    boost::algorithm::trim(params.title);

    // std::cout << "try select author\n";
    auto author_id = SelectAuthorAdvanced(true);
    if (not author_id.has_value())
        return std::nullopt;
    else {
        params.author_id = *author_id;
        params.tags = EnterTags();
        return params;
    }
}

std::optional<std::string> View::SelectAuthorAdvanced(bool NeedCreation) const {
    auto authors = GetAuthors();
    output_ << "Enter author name or empty line to select from list:" << std::endl;

    std::string str;
    // if (!std::getline(input_, str)) {
    //     return std::nullopt;
    // }


    int author_idx = 0;
    if(!std::getline(input_, str) || str.empty())
    {
        // empty logic
        PrintVector(output_, authors);
        std::string str_list;
        if (!std::getline(input_, str_list) || str_list.empty()) {
            return std::nullopt;
        }
        try {
            author_idx = std::stoi(str_list);
        } catch (std::exception const&) {
            throw std::runtime_error("Invalid author num");
        }

        --author_idx;
        if (author_idx < 0 or author_idx >= authors.size()) {
            throw std::runtime_error("Invalid author num");
        }
        // std::cout << "id:" << authors[author_idx].id << std::endl;
        // std::cout << "idx:" << author_idx << std::endl;
        return authors[author_idx].id;
    }
    else
    {
        for(auto& item : authors)
        {
            if(item.name == str)
            {
                return authors[author_idx].id;
            }
            author_idx++;
        }
        if(!NeedCreation)
        {
            return std::nullopt;
        }
        std::string add_author_mark;
        output_ << "No author found. Do you want to add " << str << " (y/n)?" << std::endl;
        if (!std::getline(input_, add_author_mark) || str.empty() || (add_author_mark != "y" && add_author_mark != "Y")) {
            output_ << "Failed to add book" << std::endl;
            return std::nullopt;
        }
        else
        {
            author_idx = 0;
            std::string name_buf;
            try {
                boost::algorithm::trim(str);
                if(str.size() == 0)
                    throw std::exception();
                name_buf = str;
                // use_cases_.AddAuthor(std::move(str));
            } catch (const std::exception&) {
                output_ << "Failed to add author"sv << std::endl;
                return std::nullopt;
            }
            authors = GetAuthors();
            for(auto& item : authors)
            {
                if(item.name == name_buf)
                {
                    return authors[author_idx].id;
                }
                author_idx++;
            }
            // return str;
            return std::nullopt;
        }
    }

    return std::nullopt;
}


std::optional<std::string> View::SelectBook() const {
    auto books = GetBooks();
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }
    return books[book_idx].book_id;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }
    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    auto test = use_cases_.ShowAuthors();
    std::vector<detail::AuthorInfo> dst_autors;
    for(int i = 0; i < test.size(); i++)
    {
        dst_autors.emplace_back(detail::AuthorInfo{test[i].second, test[i].first});
    }
    return dst_autors;
}

std::vector<show_book_t> View::GetBooks() const {
    std::vector<show_book_t> books;
    auto all_books= use_cases_.ShowBooks();
    for(int i = 0; i < all_books.size(); i++)
    {
        books.push_back({all_books[i].title, all_books[i].author_name, all_books[i].publication_year, all_books[i].book_id});
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    auto books_from_author = use_cases_.ShowAuthorBooks(author_id);
    for(int i = 0; i < books_from_author.size(); i++)
    {
        books.push_back({books_from_author[i].first, books_from_author[i].second});
    }
    return books;
}

}  // namespace ui
