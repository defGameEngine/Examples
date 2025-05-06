#include "defGameEngine.hpp"

template <class T>
struct Image
{
    static_assert(std::is_arithmetic_v<T>, "T in Image<T> is not an arithmetic type");

    Image() = default;

    Image(size_t width, size_t height)
    {
        Construct(width, height);
    }

    Image(const Image& other)
    {
        operator=(other);
    }

    Image& operator=(const Image& other)
    {
        if (other.data == data)
            return *this;

        if (width != other.width || height != other.height)
            Construct(other.width, other.height);

        std::copy(other.data, other.data + other.width * other.height, data);
        return *this;
    }

    void Construct(size_t width, size_t height)
    {
        this->width = width;
        this->height = height;

        data.reset(new T[width * height]{});
    }

    //T Get(size_t x, size_t y)
    //{
    //    return data[y * width + x];
    //}

    //T Set(size_t x, size_t y, T value)
    //{
    //    data[y * width + x] = value;
    //}

    T& operator()(size_t x, size_t y) const
    {
        if (x >= 0 && y >= 0 && x < width && y < height)
            return data[y * width + x];

        return m_NullValue;
    }

    size_t width = 0;
    size_t height = 0;

    std::unique_ptr<T[]> data;

private:
    static T m_NullValue;

};

template <class T>
T Image<T>::m_NullValue = T(0);

template <class T>
struct IntegralImage : Image<T>
{
    IntegralImage() = default;

    void CalculateSums(const Image<T>& source)
    {
        this->Construct(source.width, source.height);

        for (size_t y = 0; y < this->height; y++)
            for (size_t x = 0; x < this->width; x++)
            {
                // Add value in a current cell, add top sum, add left sum and subtract top-left sum
                this->data[y * this->width + x] = source(x, y) + (*this)(x - 1, y) + (*this)(x, y - 1) - (*this)(x - 1, y - 1);
            }
    }

    T GetRegionSum(const def::Vector2i& start, const def::Vector2i& end)
    {
        // Subtract a value from a bottom of a left side, subtract a value from a right of a top side
        // and add value from the top-left side of the region because we added it twice
        return (*this)(end.x, end.y) - (*this)(start.x - 1, end.y) - (*this)(end.x, start.y - 1) + (*this)(start.x - 1, start.y - 1);
    }
};

class IntegralImages : public def::GameEngine
{
public:
    IntegralImages()
    {
        GetWindow()->SetTitle("Integral images");
    }

private:
    Image<int> canvas;
    IntegralImage<int> integral;

    int nodeSize = 8;
    def::Vector2i canvasSize;

    def::Vector2i selectionStart;

protected:
    bool OnUserCreate() override
    {
        auto window = GetWindow();

        canvasSize.x = window->GetScreenWidth() / nodeSize;
        canvasSize.y = window->GetScreenHeight() / nodeSize;

        canvas.Construct(canvasSize.x, canvasSize.y);

        return true;
    }

    bool OnUserUpdate(float) override
    {
        auto input = GetInput();
        def::Vector2i cell = input->GetMousePosition() / nodeSize;

        if (input->GetButtonState(def::Button::LEFT).held)
        {
            int& cellValue = canvas(cell.x, cell.y);

            if (input->GetKeyState(def::Key::LEFT_CONTROL).held)
                cellValue = 0;
            else
                cellValue = 1;

            integral.CalculateSums(canvas);
        }

        int selectedCells = 0;
        bool drawRegion = false;

        def::KeyState right = input->GetButtonState(def::Button::RIGHT);

        if (right.pressed)
            selectionStart = cell;

        if (right.held)
        {
            selectedCells = integral.GetRegionSum(selectionStart, cell);
            drawRegion = true;
        }

        if (right.released)
            selectedCells = 0;

        for (size_t y = 0; y < canvasSize.y; y++)
            for (size_t x = 0; x < canvasSize.x; x++)
            {
                FillRectangle(
                    x * nodeSize, y * nodeSize,
                    nodeSize, nodeSize,
                    canvas(x, y) == 0 ? def::DARK_BLUE : def::CYAN);
            }

        if (drawRegion)
            DrawRectangle(selectionStart * nodeSize, (cell - selectionStart) * nodeSize, def::GREEN);

        DrawString(10, 10, std::to_string(selectedCells));

        return true;
    }

};

int main()
{
    IntegralImages app;

    if (app.Construct(256, 240, 4, 4))
        app.Run();
}
