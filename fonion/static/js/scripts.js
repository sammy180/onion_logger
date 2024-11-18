let currentFieldIndex = 0;
let scrollLabels = [];
let boxIds = [];

async function fetchScrollLabels() {
    try {
        const response = await fetch('/', {
            headers: {
                'X-Requested-With': 'XMLHttpRequest'
            }
        });
        const data = await response.json();
        scrollLabels = data.scroll_labels;
        boxIds = data.box_ids;
        console.log('Scroll labels and box IDs fetched:', scrollLabels, boxIds);
    } catch (error) {
        console.error('Error fetching scroll labels:', error);
    }
}

async function updateQuadrants() {
    if (scrollLabels.length === 0 || boxIds.length === 0) return;

    try {
        const field = scrollLabels[currentFieldIndex];
        const response = await fetch(`/get_data?field=${field}`);
        const data = await response.json();

        // Update each quadrant and check time difference
        for (let i = 1; i <= 4; i++) {
            const quadrant = document.querySelector(`.quadrant:nth-child(${i})`);
            if (!quadrant) {
                console.warn(`Quadrant ${i} not found in DOM`);
                continue;
            }

            const boxData = data[`box${i}`];

            quadrant.querySelector('.title').textContent = `Box ${boxIds[i - 1]}`;
            quadrant.querySelector('.parameter').textContent = `${field}: ${boxData.value}`;
            quadrant.querySelector('.status ').textContent = `status: ${boxData.status}`;
            quadrant.querySelector('.time_diff').textContent = `last meas: ${boxData.time_diff} min ago`;

            // Add or remove alert class based on time difference
            if (boxData.time_diff > 5) {
                quadrant.classList.add('alert');
            } else {
                quadrant.classList.remove('alert');
            }
        }
    } catch (error) {
        console.error('Error updating quadrants:', error);
    }
}

// Update quadrants every 15 seconds
setInterval(updateQuadrants, 10000);

// Initial update when the page loads 
window.onload = async () => {
    await fetchScrollLabels();
    updateQuadrants();
};

// Update quadrants every 15 seconds
setInterval(fetchScrollLabels, 120000);

async function nextField() {
    currentFieldIndex = (currentFieldIndex + 1) % scrollLabels.length;
    await updateQuadrants();
}

function prevField() {
    currentFieldIndex = (currentFieldIndex - 1 + scrollLabels.length) % scrollLabels.length;
    updateQuadrants();
}

// Call fetchScrollLabels on page load
document.addEventListener('DOMContentLoaded', fetchScrollLabels);